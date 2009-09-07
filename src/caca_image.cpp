/*
 * Copyright(C) 2009 Romain Bignon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <cstdlib>
#ifdef USE_CACA
	#include <caca.h>
	#include <Imlib2.h>
#endif

#include "caca_image.h"

#ifdef USE_CACA
	struct image
	{
		char *pixels;
		unsigned int w, h;
		cucul_dither_t *dither;
		void *priv;

		static struct image * load(char const * name);
		void unload();
	};
#endif /* USE_CACA */

CacaImage::CacaImage()
{}

CacaImage::CacaImage(string _path, unsigned _width, unsigned _height, unsigned _font_width, unsigned _font_height)
	: path(_path),
	  width(_width),
	  height(_height),
	  font_width(_font_width),
	  font_height(_font_height)
{
}

CacaImage::~CacaImage()
{}

string CacaImage::getIRCBuffer()
{
	if(buf.empty())
		return getIRCBuffer(width, height, font_width, font_height);
	return buf;
}

string CacaImage::getIRCBuffer(unsigned _width, unsigned _height, unsigned _font_width, unsigned _font_height)
{
#ifndef USE_CACA
	throw CacaNotLoaded();
#else
	if(buf.empty() == false &&
	   width != _width && height != _height &&
	   font_width != _font_width && font_height != _font_height)
		return buf;

	if(path.empty())
		throw CacaError();

	width = _width;
	height = _height;
	font_width = _font_width;
	font_height = _font_height;

	cucul_canvas_t *cv;
	struct image *i;

	cv = cucul_create_canvas(0, 0);
	if(!cv)
		throw CacaError();

	i = image::load(path.c_str());
	if(!i)
	{
		cucul_free_canvas(cv);
		throw CacaError();
	}

	if(!width && !height)
	{
		height = 10;
		width = height * i->w * font_height / i->h / font_width;
	}
	else if(width && !height)
		height = width * i->h * font_width / i->w / font_height;
	else if(!width && height)
		width = height * i->w * font_height / i->h / font_width;

	cucul_set_canvas_size(cv, width, height);
	cucul_set_color_ansi(cv, CUCUL_DEFAULT, CUCUL_TRANSPARENT);
	cucul_clear_canvas(cv);
	if(cucul_set_dither_algorithm(i->dither, "fstein"))
	{
		i->unload();
		free(i);
		cucul_free_canvas(cv);
		throw CacaError();
	}

	cucul_dither_bitmap(cv, 0, 0, width, height, i->dither, i->pixels);

	i->unload();
	free(i);

	size_t len;
	char* tmp;
	tmp = (char*)cucul_export_memory(cv, "irc", &len);
	if(!tmp)
		throw CacaError();

	tmp[len] = 0;
	buf = tmp;

	free(tmp);

	cucul_free_canvas(cv);

	return buf;
#endif /* USE_CACA */
}

#ifdef USE_CACA
struct image* image::load(char const * name)
{
    struct image * im = (struct image*)malloc(sizeof(struct image));
    unsigned int depth, bpp, rmask, gmask, bmask, amask;

    Imlib_Image image;

    /* Load the new image */
    image = imlib_load_image(name);

    if(!image)
    {
        free(im);
        return NULL;
    }

    imlib_context_set_image(image);
    im->pixels = (char *)imlib_image_get_data_for_reading_only();
    im->w = imlib_image_get_width();
    im->h = imlib_image_get_height();
    rmask = 0x00ff0000;
    gmask = 0x0000ff00;
    bmask = 0x000000ff;
    amask = 0xff000000;
    bpp = 32;
    depth = 4;

    /* Create the libcaca dither */
    im->dither = cucul_create_dither(bpp, im->w, im->h, depth * im->w,
                                     rmask, gmask, bmask, amask);
    if(!im->dither)
    {
        imlib_free_image();
        free(im);
        return NULL;
    }

    im->priv = (void *)image;

    return im;
}

void image::unload()
{
    /* Imlib_Image image = (Imlib_Image)im->priv; */
    imlib_free_image();
    cucul_free_dither(dither);
}
#endif /* USE_CACA */
