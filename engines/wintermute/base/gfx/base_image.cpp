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

/*
 * This file is based on WME Lite.
 * http://dead-code.org/redir.php?target=wmelite
 * Copyright (c) 2011 Jan Nedoma
 */

#include "engines/wintermute/base/gfx/base_image.h"
#include "engines/wintermute/base/base_file_manager.h"
#include "engines/wintermute/base/file/base_savefile_manager_file.h"

#include "graphics/surface.h"

#include "image/png.h"
#include "image/jpeg.h"
#include "image/bmp.h"
#include "image/tga.h"

#include "common/textconsole.h"
#include "common/stream.h"
#include "common/system.h"

namespace Wintermute {

//////////////////////////////////////////////////////////////////////
BaseImage::BaseImage() {
	_fileManager = BaseFileManager::getEngineInstance();
	_palette = nullptr;
	_paletteCount = 0;
	_surface = nullptr;
	_decoder = nullptr;
	_deletableSurface = nullptr;
}


//////////////////////////////////////////////////////////////////////
BaseImage::~BaseImage() {
	delete _decoder;
	if (_deletableSurface) {
		_deletableSurface->free();
	}
	delete _deletableSurface;
}

bool BaseImage::loadFile(const Common::String &filename) {
	_filename = filename;
	_filename.toLowercase();
	if (filename.hasPrefix("savegame:") || _filename.hasSuffix(".bmp")) {
		_decoder = new Image::BitmapDecoder();
	} else if (_filename.hasSuffix(".png")) {
		_decoder = new Image::PNGDecoder();
	} else if (_filename.hasSuffix(".tga")) {
		_decoder = new Image::TGADecoder();
	} else if (_filename.hasSuffix(".jpg")) {
		_decoder = new Image::JPEGDecoder();
	} else {
		error("BaseImage::loadFile : Unsupported fileformat %s", filename.c_str());
	}
	_filename = filename;
	Common::SeekableReadStream *file = _fileManager->openFile(filename.c_str());
	if (!file) {
		return false;
	}

	_decoder->loadStream(*file);
	_surface = _decoder->getSurface();
	_palette = _decoder->getPalette().data();
	_paletteCount = _decoder->getPalette().size();
	_fileManager->closeFile(file);

	return true;
}

byte BaseImage::getAlphaAt(int x, int y) const {
	if (!_surface) {
		return 0xFF;
	}
	uint32 color = *(const uint32 *)_surface->getBasePtr(x, y);
	byte r, g, b, a;
	_surface->format.colorToARGB(color, a, r, g, b);
	return a;
}

void BaseImage::copyFrom(const Graphics::Surface *surface) {
	_surface = _deletableSurface = new Graphics::Surface();
	_deletableSurface->copyFrom(*surface);
}

//////////////////////////////////////////////////////////////////////////
bool BaseImage::saveBMPFile(const Common::String &filename) const {
	Common::WriteStream *stream = openSfmFileForWrite(filename);
	if (stream) {
		bool ret = writeBMPToStream(stream);
		delete stream;
		return ret;
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
bool BaseImage::resize(int newWidth, int newHeight) {
	Graphics::Surface *temp = _surface->scale((uint16)newWidth, (uint16)newHeight, true);
	if (_deletableSurface) {
		_deletableSurface->free();
		delete _deletableSurface;
		_deletableSurface = nullptr;
	}
	_surface = _deletableSurface = temp;
	return true;
}


//////////////////////////////////////////////////////////////////////////
bool BaseImage::writeBMPToStream(Common::WriteStream *stream) const {
	if (!_surface) {
		return false;
	}

	/* The following is just copied over and inverted to write-ops from the BMP-decoder */
	stream->writeByte('B');
	stream->writeByte('M');

	/* Since we don't care during reads, we don't care during writes: */
	/* uint32 fileSize = */
	stream->writeUint32LE(54 + _surface->h * _surface->pitch);
	/* uint16 res1 = */
	stream->writeUint16LE(0);
	/* uint16 res2 = */
	stream->writeUint16LE(0);
	const uint32 imageOffset = 54;
	stream->writeUint32LE(imageOffset);

	const uint32 infoSize = 40; /* Windows v3 BMP */
	stream->writeUint32LE(infoSize);

	uint32 width = _surface->w;
	int32 height = _surface->h;
	stream->writeUint32LE(width);
	stream->writeUint32LE((uint32)height);

	if (width == 0 || height == 0) {
		return false;
	}

	if (height < 0) {
		warning("Right-side up bitmaps not supported");
		return false;
	}

	/* uint16 planes = */ stream->writeUint16LE(1);
	const uint16 bitsPerPixel = 24;
	stream->writeUint16LE(bitsPerPixel);

	const uint32 compression = 0;
	stream->writeUint32LE(compression);

	/* uint32 imageSize = */
	stream->writeUint32LE(_surface->h * _surface->pitch);
	/* uint32 pixelsPerMeterX = */
	stream->writeUint32LE(0);
	/* uint32 pixelsPerMeterY = */
	stream->writeUint32LE(0);
	const uint32 paletteColorCount = 0;
	stream->writeUint32LE(paletteColorCount);
	/* uint32 colorsImportant = */
	stream->writeUint32LE(0);

	// Start us at the beginning of the image (54 bytes in)
	Graphics::PixelFormat format = Graphics::PixelFormat::createFormatCLUT8();

	// BGRA for 24bpp
	if (bitsPerPixel == 24) {
		format = Graphics::PixelFormat(4, 8, 8, 8, 8, 8, 16, 24, 0);
	}

	Graphics::Surface *surface = _surface->convertTo(format);

	int srcPitch = width * (bitsPerPixel >> 3);
	const int extraDataLength = (srcPitch % 4) ? 4 - (srcPitch % 4) : 0;

	for (int32 i = height - 1; i >= 0; i--) {
		for (uint32 j = 0; j < width; j++) {
			byte b, g, r;
			uint32 color = *(uint32 *)surface->getBasePtr(j, i);
			surface->format.colorToRGB(color, r, g, b);
			stream->writeByte(b);
			stream->writeByte(g);
			stream->writeByte(r);
		}

		for (int k = 0; k < extraDataLength; k++) {
			stream->writeByte(0);
		}
	}
	surface->free();
	delete surface;
	return true;
}


//////////////////////////////////////////////////////////////////////////
bool BaseImage::copyFrom(BaseImage *origImage, int newWidth, int newHeight) {
	Graphics::Surface *temp = origImage->_surface->scale((uint16)newWidth, (uint16)newHeight, true);
	if (_deletableSurface) {
		_deletableSurface->free();
		delete _deletableSurface;
		_deletableSurface = nullptr;
	}
	_surface = _deletableSurface = temp;
	return true;
}

} // End of namespace Wintermute
