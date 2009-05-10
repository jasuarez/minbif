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

#ifndef CACA_IMAGE_H
#define CACA_IMAGE_H

#include <string>
#include <exception>

using std::string;

/** Raised when libcaca can't decode image */
class CacaError : public std::exception {};

/** Raised when libcaca isn't loaded. */
class CacaNotLoaded : public std::exception {};

/** Convert an image (JPG/PNG/..) to a beautiful ASCII-art picture. */
class CacaImage
{
	string path;
	string buf;
	unsigned width, height, font_width, font_height;

public:

	/** Default constructor */
	CacaImage();

	/** Constructor with parameters
	 *
	 * @param path  path to file
	 * @param width  render's text width
	 * @param height  render's text height
	 * @param font_width  font width
	 * @param font_height  font height
	 */
	CacaImage(string path, unsigned width = 0, unsigned height = 0, unsigned font_width = 6, unsigned font_height = 10);
	~CacaImage();

	/** Get IRC buffer to ASCII art picture.
	 * If buffer is empty, it builds it.
	 *
	 * @param width  render's text width
	 * @param height  render's text height
	 * @param font_width  font width
	 * @param font_height  font height
	 * @return  buffer of picture.
	 */
	string getIRCBuffer(unsigned width, unsigned height = 0, unsigned font_width = 6, unsigned font_height = 10);

	/** Get IRC buffer to ASCII art picture.
	 * If buffer is empty, it builds it with default parameters.
	 */
	string getIRCBuffer();
};

#endif /* CACA_IMAGE_H */
