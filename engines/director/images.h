/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef DIRECTOR_IMAGES_H
#define DIRECTOR_IMAGES_H

#include "graphics/palette.h"
#include "image/image_decoder.h"

namespace Common {
class SeekableReadStream;
struct Rect;
}

namespace Graphics {
struct Surface;
}

namespace Image {
class Codec;
}

namespace Director {

class DIBDecoder : public Image::ImageDecoder {
public:
	DIBDecoder();
	~DIBDecoder() override;

	// ImageDecoder API
	void destroy() override;
	bool loadStream(Common::SeekableReadStream &stream) override;
	const Graphics::Surface *getSurface() const override { return _surface; }
	const Graphics::Palette &getPalette() const override { return _palette; }
	void loadPalette(Common::SeekableReadStream &stream);

private:
	Image::Codec *_codec;
	const Graphics::Surface *_surface;
	Graphics::Palette _palette;
	uint16 _bitsPerPixel;
};

class BITDDecoder : public Image::ImageDecoder {
public:
	BITDDecoder(int w, int h, uint16 bitsPerPixel, uint16 pitch, const byte *palette, uint16 version);
	~BITDDecoder() override;

	// ImageDecoder API
	void destroy() override;
	bool loadStream(Common::SeekableReadStream &stream) override;
	const Graphics::Surface *getSurface() const override { return _surface; }
	const Graphics::Palette &getPalette() const override { return _palette; }
	void loadPalette(Common::SeekableReadStream &stream);

private:
	Graphics::Surface *_surface;
	Graphics::Palette _palette;
	uint16 _bitsPerPixel;
	uint16 _version;
	uint16 _pitch;
};

void copyStretchImg(const Graphics::Surface *srcSurface, Graphics::Surface *targetSurface, const Common::Rect &srcRect, const Common::Rect &targetRect, const byte *pal = 0);

} // End of namespace Director

#endif
