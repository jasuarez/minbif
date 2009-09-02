/* config.cpp - This is a library to make a configuration
 *
 * Copyright (C) 2009 Romain Bignon  <romain@symlink.me>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <fstream>
#include <iostream>
#include "config.h"

MyConfig conf;

/* Définition de quelques macros utiles */
#undef FOR
#define FOR(T, v, x) \
	T (x); \
	for(T::iterator x##it = (v).begin(); x##it != (v).end() && (x = x##it->second); ++x##it)
#undef FORit
#define FORit(T, v, x) \
	for(T::iterator (x) = (v).begin(); (x) != (v).end(); ++(x))
#define FORmm(T, v, label, x) \
	T::iterator x##lb = v.lower_bound(label); \
	T::iterator x##ub = v.upper_bound(label); \
	for(T::iterator x = x##lb; x != x##ub; ++x)
#define Error(x) do { std::cerr << path << ":" << line_count << ": " << x << std::endl ; error = true; } while(0)

std::string stringtok(std::string &in, const char * const delimiters = " \t\n");

			/********************************************************************************************
			 *                                Config                                                    *
			 ********************************************************************************************/

MyConfig::MyConfig(std::string _path)
			: path(_path), loaded(false)
{

}

MyConfig::MyConfig()
			: loaded(false)
{

}

MyConfig::~MyConfig()
{
	FORit(SectionMap, sections, it)
		delete it->second;
}

bool MyConfig::Load(std::string _path)
{
	if(!_path.empty())
		this->path = _path;

	if(path.empty())
		throw error_exc("Filename is empty");

	std::ifstream fp(path.c_str());

	if(!fp)
	{
		std::cerr << path + ": File not found" << std::endl;
		return false;
	}

	std::string ligne;
	line_count = 0;
	bool error = false;
	ConfigSection* section = 0;
	while(std::getline(fp, ligne))
	{
		++line_count;

		const char* ptr = ligne.c_str();
		while(ptr && *ptr && (*ptr == ' ' || *ptr == '\t')) ++ptr;
		ligne = ptr;

		if(ligne.empty() || ligne[0] == '#' || ligne[0] == '\r' || ligne[0] == '\n')
			continue;

		if(ligne.find('=') != std::string::npos)
		{
			if(!section)
			{
				Error("We aren't in a section !");
				continue;
			}
			std::string label = stringtok(ligne, " ");
			ConfigItem* item = section->GetItem(label);
			if(!item)
			{
				Error("Unknown item '" << label << "'");
				continue;
			}
			if(ligne.empty())
			{
				Error("There isn't any value");
				continue;
			}
			std::string value;
			ptr = ligne.c_str();
			while(ptr && *ptr && (*ptr == ' ' || *ptr == '=' || *ptr == '\t')) ++ptr;
			if(!ptr || !*ptr)
			{
				Error("There isn't any value");
				continue;
			}

			value = ptr;

			if(section->NameItem() == item)
			{
				ConfigSection* s = 0;
				if(section->Parent())
					s = section->Parent()->GetSection(section->Label(), value);
				else
					s = GetSection(section->Label(), value);

				if(s)
				{
					Error("There is already a section named '" << value << "' ('" << s->Label() << "')");
					continue;
				}
				section->SetName(value);
			}

			item->SetFound();

			if(!item->SetValue(value))
				Error("'" << value << "' is an incorrect value for '" << item->Label() << "' item (type of value is " <<
					item->ValueType() << ")");
			else if(item->CallBack())
				item->CallBack() (item);
		}
		else if(!ligne.empty() && ligne[0] == '}')
		{
			if(!section)
			{
				Error("a '}' out of a section !?");
				continue;
			}
			if(section->IsCopy() && section->Name().empty())
				Error("Section '" << section->Label() << "' hasn't a name");

			/* This function is usefull to set default values for items,
			 * and to detect if an item is missing in this section.
			 * This function will send errors.
			 */
			if(section->FindEmpty())
				error = true;
			else if(section->EndOfTab())
				section->EndOfTab() (section);
			section = section->Parent();
		}
		else
		{
			std::string tab = stringtok(ligne, " ");
			if(!section)
				section = GetSection(tab);
			else
				section = section->GetSection(tab);

			if(!section)
			{
				Error("Unknown section '" << tab << "'");
				continue;
			}
			section->SetFound();
			if(section->IsMultiple())
			{
				section = new ConfigSection(*section);
				section->SetCopy();
				if(section->Parent())
					section->Parent()->AddSection(section);
				else
					AddSection(section);
			}
		}
	}

	if(section)
		Error("in «" + section->Label() + (section->Name().empty() ? "" : ("(" + section->Name() + ")")) +
		"»: '}' not found to close section !");

	if(FindEmpty())				  // Find empty sections
		error = true;

	if(!error)
		loaded = true;
	return !error;
}

ConfigSection* MyConfig::GetSection(std::string label)
{
	FORmm(SectionMap, sections, label, it)
		if(it->second->IsCopy() == false)
		return it->second;
	return 0;
}

ConfigSection* MyConfig::GetSection(std::string label, std::string name)
{
	FORmm(SectionMap, sections, label, it)
		if(it->second->Label() == label && it->second->IsMultiple() && it->second->Name() == name)
		return it->second;

	return 0;
}

std::vector<ConfigSection*> MyConfig::GetSectionClones(std::string label)
{
	std::vector<ConfigSection*> s;
	FORmm(SectionMap, sections, label, it)
		if(it->second->Label() == label && it->second->IsMultiple() && it->second->IsCopy())
		s.push_back(it->second);

	return s;
}

ConfigSection* MyConfig::AddSection(ConfigSection* section)
{
	sections.insert(std::make_pair(section->Label(), section));
	return section;
}

ConfigSection* MyConfig::AddSection(std::string label, std::string description, bool multiple) throw(MyConfig::error_exc)
{
	if(loaded) throw error_exc("Configuration is already loaded !");

	FORit(SectionMap, sections, it)
		if(it->second->Label() == label)
		throw error_exc("Section " + label + " has a name already used");

	return AddSection(new ConfigSection(label, description, multiple, this, 0));
}

bool MyConfig::FindEmpty()
{
	std::string begin = "in global porty: ";
	bool error = false;			  // Error() macro change this value

	FORit(SectionMap, sections, it)
		if(!it->second->Found())
	{
		ConfigSection* s = it->second;
		if(s->IsMultiple() == false)
			Error(begin << "missing section '" << s->Label() << "' (" << s->Description() << ")");
	}

	return error;
}

/********************************************************************************************
 *                                ConfigSection                                             *
 ********************************************************************************************/

ConfigSection::ConfigSection(std::string _label, std::string _description, bool _multiple,
			MyConfig* _config, ConfigSection* _parent)
			: label(_label), description(_description), multiple(_multiple), config(_config), parent(_parent), copy(false),
			name_item(0), found(false), end_func(0)
{
	items.clear();
}

ConfigSection::ConfigSection(ConfigSection& cs)
			: label(cs.label), description(cs.description), multiple(cs.multiple), config(cs.config), parent(cs.parent),
			copy(cs.copy), name_item(0), found(cs.found), end_func(cs.end_func)
{
	FORit(SectionMap, cs.sections, it)
		sections.insert(std::make_pair(it->second->label, new ConfigSection(*it->second)));

	FORit(ItemMap, cs.items, it)
	{
		ConfigItem* item = it->second->Clone();
		items[it->second->Label()] = item;
		if(cs.name_item == it->second)
			name_item = item;
	}
}

ConfigSection::~ConfigSection()
{
	FORit(SectionMap, sections, it)
		delete it->second;

	FORit(ItemMap, items, it)
		delete it->second;
}

ConfigSection* ConfigSection::GetSection(std::string label)
{
	FORmm(SectionMap, sections, label, it)
		if(it->second->IsCopy() == false)
		return it->second;
	return 0;
}

ConfigSection* ConfigSection::GetSection(std::string label, std::string name)
{
	FORmm(SectionMap, sections, label, it)
		if(it->second->Label() == label && it->second->IsMultiple() && it->second->Name() == name)
		return it->second;

	return 0;
}

std::vector<ConfigSection*> ConfigSection::GetSectionClones(std::string label)
{
	std::vector<ConfigSection*> s;
	FORmm(SectionMap, sections, label, it)
		if(it->second->Label() == label && it->second->IsMultiple() && it->second->IsCopy())
		s.push_back(it->second);

	return s;
}

bool ConfigSection::FindEmpty()
{
	std::string begin = "in «" + Label() + (Name().empty() ? "" : ("(" + Name() + ")")) + "»: ";
	bool error = false;			  // Error() macro change this value
	int line_count = config->NbLines();		  /* Récuperation des informations de MyConfig */
	std::string path = config->Path();	  /* pour pouvoir utiliser la macro Error()    */

	FORit(ItemMap, items, it)
		if(!it->second->Found())
	{
		ConfigItem* item = it->second;
		if(item->DefValue().empty() || item->SetValue(item->DefValue()) == false)
			Error(begin << "missing item '" << item->Label() << "' (" << item->Description() << ")");
	}

	FORit(SectionMap, sections, it)
		if(!it->second->Found() && it->second->IsMultiple() == false)
	{
		ConfigSection* s = it->second;
		Error(begin << "missing section '" << s->Label() << "' (" << s->Description() << ")");
	}

	return error;
}

ConfigSection* ConfigSection::AddSection(ConfigSection* section)
{
	sections.insert(std::make_pair(section->Label(), section));
	return section;
}

ConfigSection* ConfigSection::AddSection(std::string label, std::string description, bool multiple) throw(MyConfig::error_exc)
{
	FORit(SectionMap, sections, it)
		if(it->second->Label() == label)
		throw MyConfig::error_exc("Section " + label + " has a name already used in section \"" + Label() + "\"");

	return AddSection(new ConfigSection(label, description, multiple, config, this));
}

ConfigItem* ConfigSection::GetItem(std::string label)
{
	ItemMap::iterator it = items.find(label);
	if(it == items.end())
		return 0;
	else
		return it->second;
}

void ConfigSection::AddItem(ConfigItem* item, bool is_name) throw(MyConfig::error_exc)
{
	try
	{
		if(!item)
			throw MyConfig::error_exc("You have to give an item !!");

		if(items[item->Label()] != 0)
			throw MyConfig::error_exc("There is already an item in \"" + Label() + "\" section named \"" + item->Label() + "\"");

		if(is_name && name_item)
			throw MyConfig::error_exc("I want to add an 'is_name' item, but I have already one !");

		item->config = config;
		item->parent = this;

		items[item->Label()] = item;
		if(is_name)
			name_item = item;
	}
	catch(...)
	{
		delete item;
		throw;
	}
}
