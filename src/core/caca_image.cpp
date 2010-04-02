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
#include <glib.h>
#ifdef HAVE_CACA
	#include <caca.h>
	#include <Imlib2.h>
#endif

#include "caca_image.h"
#include <string.h>

#ifdef HAVE_CACA
	struct CacaImage::image
	{
		char *pixels;
		unsigned int w, h;
		cucul_dither_t *dither;
		void *priv;
		unsigned ref;

		image();
		image(const image& img);
		~image();
		void create_dither(unsigned bpp);
		static struct CacaImage::image * load_file(char const * name);
	};
#endif /* HAVE_CACA */

CacaImage::CacaImage()
	: width(0),
	  height(0),
	  font_width(16),
	  font_height(10),
	  img(0)
{}

CacaImage::CacaImage(string path)
	: width(0),
	  height(0),
	  font_width(6),
	  font_height(10),
	  img(NULL)
{
#ifdef HAVE_CACA
	img = image::load_file(path.c_str());
#endif
}

CacaImage::CacaImage(void* buf, size_t size, unsigned buf_width, unsigned buf_height, unsigned bpp)
	: width(0),
	  height(0),
	  font_width(6),
	  font_height(10),
	  img(NULL)
{
#ifdef HAVE_CACA
	img = new image();
	img->w = buf_width;
	img->h = buf_height;
	img->pixels = g_strndup((char*)buf, size);

	img->create_dither(bpp);
#endif
}

CacaImage::CacaImage(const CacaImage& caca)
	: buf(caca.buf),
	  width(caca.width),
	  height(caca.height),
	  font_width(caca.font_width),
	  font_height(caca.font_height),
	  img(caca.img)
{
#ifdef HAVE_CACA
	img->ref++;
#endif
}

CacaImage& CacaImage::operator=(const CacaImage& caca)
{
	deinit();
	buf = caca.buf;
	width = caca.width;
	height = caca.height;
	font_width = caca.font_width;
	font_height = caca.font_height;
	img = caca.img;
#ifdef HAVE_CACA
	img->ref++;
#endif
	return *this;
}

CacaImage::~CacaImage()
{
	deinit();
}

void CacaImage::deinit()
{
#ifdef HAVE_CACA
	if(img)
	{
		img->ref--;
		if(img->ref < 1)
			delete img;
		img = NULL;
	}
#endif
}

string CacaImage::getIRCBuffer()
{
	if(buf.empty())
		return getIRCBuffer(width, height, "irc", font_width, font_height);
	return buf;
}

string CacaImage::getIRCBuffer(unsigned _width, unsigned _height, const char *output_type, unsigned _font_width, unsigned _font_height)
{
#ifndef HAVE_CACA
	throw CacaNotLoaded();
#else
	if(!img)
		throw CacaError();

	if(buf.empty() == false &&
	   width == _width && height == _height &&
	   font_width == _font_width && font_height == _font_height)
		return buf;

	width = _width;
	height = _height;
	font_width = _font_width;
	font_height = _font_height;

	cucul_canvas_t *cv = cucul_create_canvas(0, 0);
	if(!cv)
		throw CacaError();

	if(!width && !height)
	{
		height = 10;
		width = height * img->w * font_height / img->h / font_width;
	}
	else if(width && !height)
		height = width * img->h * font_width / img->w / font_height;
	else if(!width && height)
		width = height * img->w * font_height / img->h / font_width;

	cucul_set_canvas_size(cv, width, height);
	cucul_set_color_ansi(cv, CUCUL_DEFAULT, CUCUL_TRANSPARENT);
	cucul_clear_canvas(cv);
	if(cucul_set_dither_algorithm(img->dither, "fstein"))
	{
		cucul_free_canvas(cv);
		throw CacaError();
	}

	cucul_dither_bitmap(cv, 0, 0, width, height, img->dither, img->pixels);

	size_t len;
	char* tmp;
	tmp = (char*)cucul_export_memory(cv, output_type, &len);
	if(!tmp)
		throw CacaError();

	tmp = (char*)realloc(tmp, len+1);
	tmp[len] = 0;
	buf = tmp;

	free(tmp);

	cucul_free_canvas(cv);

	return buf;
#endif /* HAVE_CACA */
}

#ifdef HAVE_CACA
CacaImage::image::image()
	: pixels(0),
	  w(0),
	  h(0),
	  dither(0),
	  priv(0),
	  ref(1)
{
}

CacaImage::image::~image()
{
	if(priv)
		imlib_free_image();
	if(dither)
		cucul_free_dither(dither);
	free(pixels);
}

void CacaImage::image::create_dither(unsigned int bpp)
{
	unsigned int depth, rmask, gmask, bmask, amask;
	rmask = 0x00ff0000;
	gmask = 0x0000ff00;
	bmask = 0x000000ff;
	amask = 0xff000000;
	depth = bpp / 8;

	/* Create the libcaca dither */
	dither = cucul_create_dither(bpp, w, h, depth * w,
				     rmask, gmask, bmask, amask);

}

struct CacaImage::image* CacaImage::image::load_file(char const * name)
{
	struct image * im = new image();

	Imlib_Image image;

	/* Load the new image */
	image = imlib_load_image(name);

	if(!image)
	{
		delete im;
		return NULL;
	}

	imlib_context_set_image(image);
	im->w = imlib_image_get_width();
	im->h = imlib_image_get_height();
	im->pixels = (char*)malloc(im->w * im->h * 4);
	memcpy(im->pixels, imlib_image_get_data_for_reading_only(), im->w * im->h * 4);

	im->create_dither(32);
	if(!im->dither)
	{
		delete im;
		return NULL;
	}

	im->priv = (void *)image;

	return im;
}

#endif /* HAVE_CACA */
