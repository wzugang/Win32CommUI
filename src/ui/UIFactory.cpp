#include "UIFactory.h"
#include "XmlParser.h"
#include "XComponent.h"
#include "VComponent.h"
#include "XExt.h"
#include "VExt.h"
#include "XBinFile.h"
#include <string.h>
#include <map>
#include <string>
#include <atlimage.h>

static std::map<std::string, XImage*> mCache;

static XImage *findInCache(const char *name) {
	std::map<std::string, XImage*>::iterator it = mCache.find(name);
	if (it == mCache.end())
		return NULL;
	return it->second;
}
static char *getCacheName(ResPath *r, bool basic) {
	static char CACHE_NAME[128];
	if (!r->mHasRect || basic) {
		sprintf(CACHE_NAME, "%s", r->mPath);
	} else {
		sprintf(CACHE_NAME, "%s [%d %d %d %d]", r->mPath, r->mX, r->mY, r->mWidth, r->mHeight);
	}
	return CACHE_NAME;
}

XImage::XImage(HBITMAP bmp, int w, int h, void *bits, int bitPerPix, int rowBytes) {
	mHBitmap = bmp;
	mWidth = w;
	mHeight = h;
	mBits = bits;
	mBitPerPix = bitPerPix;
	mRowBytes = rowBytes;
	mHasAlphaChannel = (bitPerPix == 32);
	mTransparentColor = -1;
	mRepeatX = mRepeatY = mStretch = m9Patch = false;
	mDeleteBitmap = false;
}

XImage * XImage::load( const char *resPath ) {
	ResPath info;
	if (! info.parse(resPath)) {
		return NULL;
	}
	XImage *img = findInCache(getCacheName(&info, false));
	if (img != NULL) {
		goto _end;
	}
	img = findInCache(getCacheName(&info, true));
	if (img == NULL) {
		img = loadImage(&info);
		if (img == NULL) {
			return NULL;
		}
		mCache[getCacheName(&info, true)] = img;
	}
	if (! info.mHasRect) {
		goto _end;
	}
	img = createPart(img, info.mX, info.mY, info.mWidth, info.mHeight);
	if (img == NULL)
		return NULL;
	mCache[getCacheName(&info, false)] = img;
_end:
	img = new XImage(img->mHBitmap, img->mWidth, img->mHeight, img->mBits, img->mBitPerPix, img->mRowBytes);
	img->mRepeatX = info.mRepeatX;
	img->mRepeatY = info.mRepeatY;
	img->mStretch = info.mStretch;
	img->m9Patch = info.m9Patch;
	return img;
}

XImage * XImage::loadImage( ResPath *info) {
	if (info->mResType == ResPath::RT_COLOR) {
		COLORREF color = 0;
		if (! AttrUtils::parseColor(info->mPath, &color)) {
			return NULL;
		}
		bool hasAlpha = ((color >> 24) & 0xff) != 0xff;
		XImage *img = create(info->mWidth, info->mHeight, (hasAlpha ? 32 : 24));
		img->fillColor(color);
		return img;
	}

	CImage cimg;
	if (info->mResType == ResPath::RT_FILE) {
		cimg.Load(info->mPath);
	} else if (info->mResType == ResPath::RT_XBIN) {
		int len = 0;
		void *data = XBinFile::getInstance()->find(info->mPath, &len);
		HGLOBAL m_hMem = GlobalAlloc(GMEM_FIXED, len);
		BYTE* pmem = (BYTE*)GlobalLock(m_hMem);
		memcpy(pmem, data, len);
		IStream* pstm = NULL;
		CreateStreamOnHGlobal(m_hMem, FALSE, &pstm);  
		cimg.Load(pstm);
		GlobalUnlock(m_hMem);
		pstm->Release();
	} else if (info->mResType == ResPath::RT_RES) {
		cimg.LoadFromResource(XComponent::getInstance(), info->mPath);
	}
	if (cimg.GetWidth() == 0) {
		return NULL;
	}

	XImage *img = new XImage(NULL, cimg.GetWidth(), cimg.GetHeight(), cimg.GetBits(), cimg.GetBPP(), cimg.GetPitch());
	img->mHBitmap = cimg.Detach();
	if (img->mHasAlphaChannel) {
		img->buildAlphaChannel();
	}
	return img;
}

XImage * XImage::createPart( XImage *org, int x, int y, int width, int height ) {
	if (org == NULL) 
		return NULL;
	if (x < 0 || y < 0 || width <= 0 || height <= 0)
		return NULL;
	if (x + width > org->mWidth || y + height > org->mHeight)
		return NULL;
	XImage *img = create(width, height, org->mBitPerPix);
	for (int i = 0, r = y; i < height; ++i, ++r) {
		BYTE* src = (BYTE*)org->getRowBits(r) + x * org->mBitPerPix / 8;
		BYTE* dst = (BYTE*)img->getRowBits(i);
		memcpy(dst, src, org->mBitPerPix / 8 * width);
	}
	img->mDeleteBitmap = false;
	return img;
}

void * XImage::getRowBits( int row ) {
	if (mBits == NULL || row >= mHeight)
		return NULL;
	return (BYTE*)mBits + (mRowBytes * row);
}

XImage * XImage::create( int width, int height, int bitPerPix ) {
	int rowByteNum = width * bitPerPix / 8;
	rowByteNum = (rowByteNum + 3) / 4 * 4;
	BITMAPINFOHEADER header = {0};
	header.biSize = sizeof(BITMAPINFOHEADER);
	header.biWidth = width;
	header.biHeight = -height;
	header.biPlanes = 1;
	header.biBitCount = bitPerPix;
	header.biCompression = BI_RGB;
	header.biClrUsed = 0;
	header.biSizeImage = rowByteNum * height;
	PVOID pvBits = NULL;
	HBITMAP bmp = CreateDIBSection(NULL, (PBITMAPINFO)&header, DIB_RGB_COLORS, &pvBits, NULL, 0);
	if (bmp == NULL)
		return NULL;
	XImage *img = new XImage(bmp, width, height, pvBits, bitPerPix, rowByteNum);
	img->mDeleteBitmap = true;
	return img;
}

void XImage::buildAlphaChannel() {
	for (int r = 0; r < mHeight; ++r) {
		BYTE *p = (BYTE *)getRowBits(r);
		for (int c = 0; c < mWidth; ++c, p += 4) {
			if (p[3] == 255) continue;
			p[0] = p[0] * p[3] / 255;
			p[1] = p[1] * p[3] / 255;
			p[2] = p[2] * p[3] / 255;
		}
	}
}

HBITMAP XImage::getHBitmap() {
	return mHBitmap;
}

void * XImage::getBits() {
	return mBits;
}

int XImage::getWidth() {
	return mWidth;
}

int XImage::getHeight() {
	return mHeight;
}

XImage::~XImage() {
	if (mHBitmap && mDeleteBitmap) 
		DeleteObject(mHBitmap);
}

bool XImage::hasAlphaChannel() {
	return mHasAlphaChannel;
}

void XImage::draw( HDC dc, int destX, int destY, int destW, int destH ) {
	if (mHBitmap == NULL) return;
	HDC memDc = CreateCompatibleDC(dc);
	SelectObject(memDc, mHBitmap);
	if (mRepeatX) {
		drawRepeatX(dc, destX, destY, destW, destH, memDc);
	} else if (mRepeatY) {
		drawRepeatY(dc, destX, destY, destW, destH, memDc);
	} else if (mStretch) {
		drawStretch(dc, destX, destY, destW, destH, memDc);
	} else if (m9Patch) {
		draw9Patch(dc, destX, destY, destW, destH, memDc);
	} else {
		drawNormal(dc, destX, destY, destW, destH, memDc);
	}
	DeleteObject(memDc);
}

void XImage::draw(XImage *src, int destX, int destY, int destW, int destH, DrawAction a) {
	if (src == NULL || destX < 0 || destY < 0 || destW <= 0 || destH <= 0) {
		return;
	}
	if (destX + destW > mWidth) {
		destW = mWidth - destX;
	}
	if (destY + destH > mHeight) {
		destH = mHeight - destY;
	}
	if (destW <= 0 || destH <= 0) {
		return;
	}
	if (src->mRepeatX) {
		drawRepeatX(src, destX, destY, destW, destH, a);
	} else if (src->mRepeatY) {
		drawRepeatY(src, destX, destY, destW, destH, a);
	} else if (src->mStretch) {
		drawStretch(src, destX, destY, destW, destH, a);
	} else if (src->m9Patch) {
		draw9Patch(src, destX, destY, destW, destH, a);
	} else {
		drawNormal(src, destX, destY, destW, destH, a);
	}

	/*if (a == DA_COPY || src->mBitPerPix != 32 || mBitPerPix != 32) {
		drawCopy(src, dstX, dstY, dstW, dstH, srcX, srcY);
	} else if (a == DA_ALPHA_BLEND) {
		drawAlphaBlend(src, dstX, dstY, dstW, dstH, srcX, srcY);
	}*/
}

void XImage::drawRepeatX( HDC dc, int destX, int destY, int destW, int destH, HDC memDc ) {
	for (int w = 0, lw = destW; w < destW; w += mWidth, lw -= mWidth) {
		if (hasAlphaChannel()) {
			BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
			AlphaBlend(dc, destX + w, destY, min(mWidth, lw), min(destH, mHeight), 
				memDc, 0, 0, min(mWidth, lw), min(destH, mHeight), bf);
		} else {
			BitBlt(dc, destX + w, destY, min(mWidth, lw), min(destH, mHeight), memDc, 0, 0, SRCCOPY);
		}
	}
}

void XImage::drawRepeatX(XImage *src, int destX, int destY, int destW, int destH, DrawAction a) {
	for (int w = 0, lw = destW; w < destW; w += src->mWidth, lw -= src->mWidth) {
		if (a == DA_COPY || src->mBitPerPix != 32 || mBitPerPix != 32) {
			drawCopy(src, destX + w, destY, min(src->mWidth, lw), min(destH, src->mHeight), 0, 0);
		} else {
			drawAlphaBlend(src, destX + w, destY, min(src->mWidth, lw), min(destH, src->mHeight), 0, 0);
		}
	}
}

void XImage::drawRepeatY( HDC dc, int destX, int destY, int destW, int destH, HDC memDc ) {
	for (int h = 0, lh = destH; h < destH; h += mHeight, lh -= mHeight) {
		if (hasAlphaChannel()) {
			BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
			AlphaBlend(dc, destX, destY + h, min(destW, mWidth), min(mHeight, lh),
				memDc, 0, 0, min(destW, mWidth), min(mHeight, lh), bf);
		} else {
			BitBlt(dc, destX, destY + h, min(destW, mWidth), min(mHeight, lh), memDc, 0, 0, SRCCOPY);
		}
	}
}

void XImage::drawRepeatY(XImage *src, int destX, int destY, int destW, int destH, DrawAction a) {
	for (int h = 0, lh = destH; h < destH; h += src->mHeight, lh -= src->mHeight) {
		if (a == DA_COPY || src->mBitPerPix != 32 || mBitPerPix != 32) {
			drawCopy(src, destX, destY + h, min(destW, src->mWidth), min(src->mHeight, lh), 0, 0);
		} else {
			drawAlphaBlend(src, destX, destY + h, min(destW, src->mWidth), min(src->mHeight, lh), 0, 0);
		}
	}
}

void XImage::drawStretch( HDC dc, int destX, int destY, int destW, int destH, HDC memDc ) {
	if (hasAlphaChannel()) {
		BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
		AlphaBlend(dc, destX, destY, destW, destH, memDc, 0, 0, mWidth, mHeight, bf);
	} else {
		StretchBlt(dc, destX, destY, destW, destH, memDc, 0, 0, mWidth, mHeight, SRCCOPY);
	}
}

void XImage::drawStretch(XImage *src, int destX, int destY, int destW, int destH, DrawAction a) {
	XRect srcRect(0, 0, src->mWidth, src->mHeight);
	XRect dstRect(destX, destY, destW, destH);
	drawStretch(src, srcRect, dstRect, a);
}

void XImage::drawStretch(XImage *src, const XRect &srcRect, const XRect &destRect, DrawAction a) {
	if (src == NULL || !srcRect.isValid() || !destRect.isValid()) {
		return;
	}
	int sw = srcRect.mWidth - 1, sh = srcRect.mHeight - 1, dw = destRect.mWidth - 1, dh = destRect.mHeight - 1;
	int B, N, x, y;
	int nPixelSize = src->mBitPerPix / 8;
	BYTE * pLinePrev, *pLineNext;
	BYTE * pDest;
	BYTE * pA, *pB, *pC, *pD;
	for ( int i = 0; i <= dh; ++i ) {
		pDest = ( BYTE * )getRowBits(i + destRect.mY) + destRect.mX * (mBitPerPix / 8);
		y = i * sh / dh;
		N = dh - i * sh % dh;
		pLinePrev = ( BYTE * )src->getRowBits(srcRect.mY + y++);
		pLineNext = ( N == dh ) ? pLinePrev : ( BYTE * )src->getRowBits(srcRect.mY + y);
		for ( int j = 0; j <= dw; ++j ) {
			x = (j * sw / dw + srcRect.mX)* nPixelSize;
			B = dw - j * sw % dw;
			pA = pLinePrev + x;
			pB = pA + nPixelSize;
			pC = pLineNext + x;
			pD = pC + nPixelSize;
			if ( B == dw ) {
				pB = pA;
				pD = pC;
			}
			BYTE alpha = 255;
			bool doAlpha = (a == DA_ALPHA_BLEND && src->mBitPerPix == 32 && mBitPerPix == 32);
			if (doAlpha) {
				alpha = pA[3];
			}
			for ( int k = 0; k < mBitPerPix / 8 && k < 3; ++k ) {
				BYTE dd = ( BYTE )( int )(
				( B * N * ( *pA++ - *pB - *pC + *pD ) + dw * N * *pB++
				+ dh * B * *pC++ + ( dw * dh - dh * B - dw * N ) * *pD++
				+ dw * dh / 2 ) / ( dw * dh ) );
				
				if (! doAlpha) {
					pDest[k] = dd;
				} else {
					pDest[k] = dd + ((255 - alpha) * pDest[k]) / 255;
				}
			}
			if (! doAlpha && mBitPerPix == 32) {
				pDest[3] = 255;
			}
			pDest += (mBitPerPix / 8);
		}
	}
}

void XImage::draw9Patch( HDC dc, int destX, int destY, int destW, int destH, HDC memDc ) {
	if (mWidth <= 0 || mHeight <= 0) return;
	// draw left-top corner
	if (hasAlphaChannel()) {
		BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
		AlphaBlend(dc, destX, destY, mWidth/3, mHeight/3,
			memDc, 0, 0, mWidth/3, mHeight/3, bf);
	} else {
		BitBlt(dc, destX, destY, mWidth/3, mHeight/3, memDc, 0, 0, SRCCOPY);
	}
	// draw right-top corner
	if (hasAlphaChannel()) {
		BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
		AlphaBlend(dc, destX + destW - mWidth/3, destY, mWidth/3, mHeight/3,
			memDc, mWidth-mWidth/3, 0, mWidth/3, mHeight/3, bf);
	} else {
		BitBlt(dc, destX + destW - mWidth/3, destY, mWidth/3, mHeight/3, memDc, mWidth-mWidth/3, 0, SRCCOPY);
	}
	// draw left-bottom corner
	if (hasAlphaChannel()) {
		BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
		AlphaBlend(dc, destX, destY + destH - mHeight/3, mWidth/3, mHeight/3,
			memDc, 0, mHeight-mHeight/3, mWidth/3, mHeight/3, bf);
	} else {
		BitBlt(dc, destX, destY + destH - mHeight/3, mWidth/3, mHeight/3, memDc, 0, mHeight-mHeight/3, SRCCOPY);
	}
	// draw right-bottom corner
	if (hasAlphaChannel()) {
		BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
		AlphaBlend(dc, destX + destW - mWidth/3, destY + destH - mHeight/3, mWidth/3, mHeight/3,
			memDc, mWidth-mWidth/3, mHeight-mHeight/3, mWidth/3, mHeight/3, bf);
	} else {
		BitBlt(dc, destX + destW - mWidth/3, destY + destH - mHeight/3, mWidth/3, mHeight/3, memDc, mWidth-mWidth/3, mHeight-mHeight/3, SRCCOPY);
	}
	int cw = destW - mWidth / 3 * 2;
	int ch = destH - mHeight / 3 * 2;
	// draw top center
	if (hasAlphaChannel()) {
		BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
		AlphaBlend(dc, destX+mWidth/3, destY, destW-mWidth/3*2, mHeight/3, memDc, mWidth/3, 0, mWidth/3, mHeight/3, bf);
	} else {
		StretchBlt(dc, destX+mWidth/3, destY, destW-mWidth/3*2, mHeight/3, memDc, mWidth/3, 0, mWidth/3, mHeight/3, SRCCOPY);
	}
	// draw bottom center
	if (hasAlphaChannel()) {
		BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
		AlphaBlend(dc, destX+mWidth/3, destY+destH-mHeight/3, destW-mWidth/3*2, mHeight/3, 
			memDc, mWidth/3, mHeight-mHeight/3, mWidth/3, mHeight/3, bf);
	} else {
		StretchBlt(dc, destX+mWidth/3, destY+destH-mHeight/3, destW-mWidth/3*2, mHeight/3, 
			memDc, mWidth/3, mHeight-mHeight/3, mWidth/3, mHeight/3, SRCCOPY);
	}
	// draw left center
	if (hasAlphaChannel()) {
		BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
		AlphaBlend(dc, destX, destY+mHeight/3, mWidth/3, destH-mHeight/3*2, 
			memDc, 0, mHeight/3, mWidth/3, mHeight-mHeight/3*2, bf);
	} else {
		StretchBlt(dc, destX, destY+mHeight/3, mWidth/3, destH-mHeight/3*2, 
			memDc, 0, mHeight/3, mWidth/3, mHeight-mHeight/3*2, SRCCOPY);
	}
	// draw right center
	if (hasAlphaChannel()) {
		BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
		AlphaBlend(dc, destX+destW-mWidth/3, destY+mHeight/3, mWidth/3, destH-mHeight/3*2, 
			memDc, mWidth-mWidth/3, mHeight/3, mWidth/3, mHeight-mHeight/3*2, bf);
	} else {
		StretchBlt(dc, destX+destW-mWidth/3, destY+mHeight/3, mWidth/3, destH-mHeight/3*2, 
			memDc, mWidth-mWidth/3, mHeight/3, mWidth/3, mHeight-mHeight/3*2, SRCCOPY);
	}
	// draw center
	if (hasAlphaChannel()) {
		BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
		AlphaBlend(dc, destX+mWidth/3, destY+mHeight/3, destW-mWidth/3*2, destH-mHeight/3*2, 
			memDc, mWidth/3, mHeight/3, mWidth-mWidth/3*2, mHeight-mHeight/3*2, bf);
	} else {
		StretchBlt(dc, destX+mWidth/3, destY+mHeight/3, destW-mWidth/3*2, destH-mHeight/3*2, 
			memDc, mWidth/3, mHeight/3, mWidth-mWidth/3*2, mHeight-mHeight/3*2, SRCCOPY);
	}
}

void XImage::draw9Patch(XImage *src, int destX, int destY, int destW, int destH, DrawAction a) {
	if (src->mWidth <= 0 || src->mHeight <= 0) {
		return;
	}
	bool doAlpha = (a == DA_ALPHA_BLEND && src->mBitPerPix == 32 && mBitPerPix == 32);
	// draw left-top corner
	if (doAlpha) {
		drawAlphaBlend(src, destX, destY, src->mWidth/3, src->mHeight/3, 0, 0);
	} else {
		drawCopy(src, destX, destY, src->mWidth/3, src->mHeight/3, 0, 0);
	}
	// draw right-top corner
	if (doAlpha) {
		drawAlphaBlend(src, destX + destW - src->mWidth/3, destY, src->mWidth/3, src->mHeight/3, src->mWidth-src->mWidth/3, 0);
	} else {
		drawCopy(src, destX + destW - src->mWidth/3, destY, src->mWidth/3, src->mHeight/3, src->mWidth-src->mWidth/3, 0);
	}
	// draw left-bottom corner
	if (doAlpha) {
		drawAlphaBlend(src, destX, destY + destH - src->mHeight/3, src->mWidth/3, src->mHeight/3, 0, src->mHeight-src->mHeight/3);
	} else {
		drawCopy(src, destX, destY + destH - src->mHeight/3, src->mWidth/3, src->mHeight/3, 0, src->mHeight-src->mHeight/3);
	}
	// draw right-bottom corner
	if (doAlpha) {
		drawAlphaBlend(src, destX + destW - src->mWidth/3, destY + destH - src->mHeight/3, src->mWidth/3, src->mHeight/3, src->mWidth-src->mWidth/3, src->mHeight-src->mHeight/3);
	} else {
		drawCopy(src, destX + destW - src->mWidth/3, destY + destH - src->mHeight/3, src->mWidth/3, src->mHeight/3, src->mWidth-src->mWidth/3, src->mHeight-src->mHeight/3);
	}

	XRect srcRect, dstRect;
	int cw = destW - src->mWidth / 3 * 2;
	int ch = destH - src->mHeight / 3 * 2;
	// draw top center
	dstRect.set(destX+src->mWidth/3, destY, cw, src->mHeight/3);
	srcRect.set(src->mWidth/3, 0, src->mWidth/3, src->mHeight/3);
	drawStretch(src, srcRect, dstRect, a);
	
	// draw bottom center
	dstRect.set(destX+src->mWidth/3, destY+destH-src->mHeight/3, destW-src->mWidth/3*2, src->mHeight/3);
	srcRect.set(src->mWidth/3, src->mHeight-src->mHeight/3, src->mWidth/3, src->mHeight/3);
	drawStretch(src, srcRect, dstRect, a);

	// draw left center
	dstRect.set(destX, destY+src->mHeight/3, src->mWidth/3, destH-src->mHeight/3*2);
	srcRect.set(0, src->mHeight/3, src->mWidth/3, src->mHeight-src->mHeight/3*2);
	drawStretch(src, srcRect, dstRect, a);

	// draw right center
	dstRect.set(destX+destW-src->mWidth/3, destY+src->mHeight/3, src->mWidth/3, destH-src->mHeight/3*2);
	srcRect.set(src->mWidth-src->mWidth/3, src->mHeight/3, src->mWidth/3, src->mHeight-src->mHeight/3*2);
	drawStretch(src, srcRect, dstRect, a);

	// draw center
	dstRect.set(destX+src->mWidth/3, destY+src->mHeight/3, destW-src->mWidth/3*2, destH-src->mHeight/3*2);
	srcRect.set(src->mWidth/3, src->mHeight/3, src->mWidth-src->mWidth/3*2, src->mHeight-src->mHeight/3*2);
	drawStretch(src, srcRect, dstRect, a);
}

void XImage::drawNormal( HDC dc, int destX, int destY, int destW, int destH, HDC memDc ) {
	if (hasAlphaChannel())  {
		BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
		AlphaBlend(dc, destX, destY, min(destW, mWidth), min(destH, mHeight),
			memDc, 0, 0,min(destW, mWidth), min(destH, mHeight), bf);
	} else {
		BitBlt(dc, destX, destY, min(destW, mWidth), min(destH, mHeight), memDc, 0, 0, SRCCOPY);
	}
}

void XImage::drawNormal(XImage *src, int destX, int destY, int destW, int destH, DrawAction a) {
	if (a == DA_COPY || src->mBitPerPix != 32 || mBitPerPix != 32) {
		drawCopy(src, destX, destY, min(destW, src->mWidth), min(destH, src->mHeight), 0, 0);
	} else if (a == DA_ALPHA_BLEND) {
		drawAlphaBlend(src, destX, destY, min(destW, src->mWidth), min(destH, src->mHeight), 0, 0);
	}
}

HICON XImage::loadIcon( const char *resPath ) {
	ResPath info;
	if (! info.parse(resPath)) {
		return NULL;
	}
	CImage cimg;
	if (info.mResType == ResPath::RT_FILE) {
		cimg.Load(info.mPath);
	} else if (info.mResType == ResPath::RT_XBIN) {
		int len = 0;
		void *data = XBinFile::getInstance()->find(info.mPath, &len);
		HGLOBAL m_hMem = GlobalAlloc(GMEM_FIXED, len);
		BYTE* pmem = (BYTE*)GlobalLock(m_hMem);
		memcpy(pmem, data, len);
		IStream* pstm = NULL;
		CreateStreamOnHGlobal(m_hMem, FALSE, &pstm);
		cimg.Load(pstm);
		GlobalUnlock(m_hMem);
		pstm->Release();
	} else {
		cimg.LoadFromResource(XComponent::getInstance(), info.mPath);
	}
	// HBITMAP bp = cimg; // cimg.operator HBITMAP();
	return (HICON)cimg.Detach();
}

void XImage::fillAlpha(BYTE alpha) {
	if (! mHasAlphaChannel) {
		return;
	}
	for (int r = 0; r < mHeight; ++r) {
		BYTE *p = (BYTE *)getRowBits(r);
		for (int c = 0; c < mWidth; ++c, p += 4) {
			p[3] = alpha;
		}
	}
}

void XImage::fillColor(COLORREF rgba) {
	BYTE r = rgba & 0xff, g = (rgba >> 8) & 0xff, b = (rgba >> 16) & 0xff, a = (rgba >> 24) & 0xff;
	if (mHasAlphaChannel) {
		rgba = (a << 24) | ((b * a / 255) << 16) | ((g * a / 255) << 8) | (r * a / 255);
		for (int r = 0; r < mHeight; ++r) {
			DWORD *p = (DWORD *)getRowBits(r);
			for (int c = 0; c < mWidth; ++c, ++p) {
				*p = rgba;
			}
		}
	} else {
		for (int r = 0; r < mHeight; ++r) {
			BYTE *p = (BYTE *)getRowBits(r);
			for (int c = 0; c < mWidth; ++c, p += 3) {
				p[0] = r;
				p[1] = g;
				p[2] = b;
			}
		}
	}
}

void XImage::drawCopy(XImage *src, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY) {
	const int SRC_STEP = src->mBitPerPix / 8;
	const int DST_STEP = mBitPerPix / 8;
	if (SRC_STEP < 3 || DST_STEP < 3) {
		return;
	}
	for (int r = 0; r < dstH; ++r) {
		BYTE *s = (BYTE *)src->getRowBits(srcY + r) + srcX * SRC_STEP;
		BYTE *d = (BYTE *)getRowBits(dstY + r) + dstX * DST_STEP;
		if (SRC_STEP == DST_STEP) {
			memcpy(d, s, dstW * DST_STEP);
		} else {
			for (int c = 0; c < dstW; ++c) {
				d[0] = s[0];
				d[1] = s[1];
				d[2] = s[2];
				if (DST_STEP == 4) {
					d[3] = 255;
				}
				s += SRC_STEP;
				d += DST_STEP;
			}
		}
	}
}

void XImage::drawAlphaBlend(XImage *src, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY) {
	const int SRC_STEP = src->mBitPerPix / 8;
	const int DST_STEP = mBitPerPix / 8;
	if (SRC_STEP != 4 || DST_STEP < 3) {
		return;
	}
	for (int r = 0; r < dstH; ++r) {
		BYTE *s = (BYTE *)src->getRowBits(srcY + r) + srcX * SRC_STEP;
		BYTE *d = (BYTE *)getRowBits(dstY + r) + dstX * DST_STEP;
		for (int c = 0; c < dstW; ++c) {
			// d[0] = (s[0] * s[3] + (255 - s[3]) * d[0]) / 255;
			// d[1] = (s[1] * s[3] + (255 - s[3]) * d[1]) / 255;
			// d[2] = (s[2] * s[3] + (255 - s[3]) * d[2]) / 255;
			d[0] = s[0] + (255 - s[3]) * d[0] / 255;
			d[1] = s[1] + (255 - s[3]) * d[1] / 255;
			d[2] = s[2] + (255 - s[3]) * d[2] / 255;
			s += SRC_STEP;
			d += DST_STEP;
		}
	}
}


//----------------------------UIFactory-------------------------
struct NodeCreator {
	NodeCreator() {
		mNodeName[0] = 0;
		mCreator = NULL;
	}
	char mNodeName[32];
	UIFactory::Creator mCreator;
};

static NodeCreator g_creators[64];
static int g_creatorNum = 0;

XComponent* UIFactory::buildComponent( XmlNode *root) {
	if (root == NULL) return NULL;
	Creator c = getCreator(root->getName());
	if (c == NULL) {
		printf("UIFactory.buildingTree: node name [%s] has no creator\n", root->getName());
		return NULL;
	}
	XComponent *x = c(root);
	root->setComponent(x);
	for (int i = 0; i < root->getChildCount(); ++i) {
		XmlNode *child = root->getChild(i);
		XComponent *ix = buildComponent(child);
		child->setComponent(ix);
	}
	return x;
}

XmlNode* UIFactory::buildNode( const char *resPath, const char *partName ) {
	if (resPath == NULL || partName == NULL)
		return NULL;
	XmlPartLoader *loader = XmlPartLoader::fetch(resPath);
	if (loader == NULL) return NULL;
	XmlPartLoader::PartItem * item = loader->getPartXml(partName);
	if (item == NULL) return NULL;
	XmlParser *parser = XmlParser::create(resPath);
	parser->parseString(item->mContent);
	XmlNode *rootNode = parser->getRoot();
	return rootNode;
}

XComponent* UIFactory::fastBuild( const char *resPath, const char *partName, XComponent *parent ) {
	XmlNode *root = UIFactory::buildNode(resPath, partName);
	if (root == NULL) return NULL;
	if (parent) root->setParent(parent->getNode());
	XComponent *cc = UIFactory::buildComponent(root);
	cc->createWndTree();
	return cc;
}

void UIFactory::registCreator( const char *nodeName, Creator c ) {
	if (nodeName == NULL || c == NULL)
		return;
	strcpy(g_creators[g_creatorNum].mNodeName, nodeName);
	g_creators[g_creatorNum].mCreator = c;
	++g_creatorNum;
}

UIFactory::Creator UIFactory::getCreator( const char *nodeName ) {
	if (nodeName == NULL) return NULL;
	for (int i = 0; i < g_creatorNum; ++i) {
		if (strcmp(nodeName, g_creators[i].mNodeName) == 0)
			return g_creators[i].mCreator;
	}
	printf("Node: [%s] has no Creator\n", nodeName);
	return NULL;
}

void UIFactory::destory( XmlNode *root ) {
	for (int i = 0; i < root->getChildCount(); ++i) {
		XmlNode *child = root->getChild(i);
		destory(child);
	}
	delete root->getComponent();
	delete root;
}
static void BuildMenu(XExtMenuModel *menu, XmlNode *menuNode) {
	for (int i = 0; i < menuNode->getChildCount(); ++i) {
		XmlNode *child = menuNode->getChild(i);
		XExtMenuItem *item = new XExtMenuItem(NULL, NULL);
		for (int j = 0; j < child->getAttrsCount(); ++j) {
			XmlNode::Attr *a = child->getAttr(j);
			if (strcmp(a->mName, "name") == 0) strcpy(item->mName, a->mValue);
			else if (strcmp(a->mName, "text") == 0) item->mText = a->mValue;
			else if (strcmp(a->mName, "active") == 0) item->mActive = AttrUtils::parseBool(a->mValue);
			else if (strcmp(a->mName, "visible") == 0) item->mVisible = AttrUtils::parseBool(a->mValue);
			else if (strcmp(a->mName, "checkable") == 0) item->mCheckable = AttrUtils::parseBool(a->mValue);
			else if (strcmp(a->mName, "checked") == 0) item->mChecked = AttrUtils::parseBool(a->mValue);
			else if (strcmp(a->mName, "separator") == 0) item->mSeparator = AttrUtils::parseBool(a->mValue);
		}
		menu->add(item);
		if (child->getChildCount() > 0) {
			XExtMenuModel *sub = new XExtMenuModel();
			item->mChild = sub;
			BuildMenu(sub, child);
		}
	}
}

XExtMenuModel * UIFactory::buildMenu( XmlNode *rootMenu ) {
	if (rootMenu == NULL) return NULL;
	XExtMenuModel *menu = new XExtMenuModel();
	BuildMenu(menu, rootMenu);
	return menu;
}

XExtMenuModel* UIFactory::fastMenu( const char *resPath, const char *partName ) {
	XmlNode *root = buildNode(resPath, partName);
	if (root == NULL) return NULL;
	XExtMenuModel *cc = buildMenu(root);
	return cc;
}
static void BuildTree(XExtTreeNode *tn, XmlNode *node) {
	for (int i = 0; i < node->getChildCount(); ++i) {
		XmlNode *child = node->getChild(i);
		XExtTreeNode *sub = new XExtTreeNode(child->getAttrValue("text"));
		sub->setExpand(AttrUtils::parseBool(child->getAttrValue("expand")));
		sub->setCheckable(AttrUtils::parseBool(child->getAttrValue("checkable")));
		sub->setChecked(AttrUtils::parseBool(child->getAttrValue("checked")));
		sub->setUserData(node);
		tn->insert(-1, sub);
		if (child->getChildCount() > 0) {
			BuildTree(sub, child);
		}
	}
}
XExtTreeNode * UIFactory::buildTree( XmlNode *rootTree ) {
	if (rootTree == NULL) return NULL;
	XExtTreeNode *node = new XExtTreeNode("Root");
	BuildTree(node, rootTree);
	return node;
}
XExtTreeNode* UIFactory::fastTree(const char *resPath, const char *partName) {
	XmlNode *root = buildNode(resPath, partName);
	if (root == NULL) return NULL;
	XExtTreeNode *cc = buildTree(root);
	return cc;
}

static XComponent *XExtEmptyComponent_Creator(XmlNode *n) {return new XExtEmptyComponent(n);}
static XComponent *XAbsLayout_Creator(XmlNode *n) {return new XAbsLayout(n);}
static XComponent *XHLineLayout_Creator(XmlNode *n) {return new XHLineLayout(n);}
static XComponent *XVLineLayout_Creator(XmlNode *n) {return new XVLineLayout(n);}

static XComponent *XWindow_Creator(XmlNode *n) {return new XWindow(n);}
static XComponent *XDialog_Creator(XmlNode *n) {return new XDialog(n);}

static XComponent *XExtButton_Creator(XmlNode *n) {return new XExtButton(n);}
static XComponent *XExtOption_Creator(XmlNode *n) {return new XExtOption(n);}
static XComponent *XExtLabel_Creator(XmlNode *n) {return new XExtLabel(n);}
static XComponent *XExtCheckBox_Creator(XmlNode *n) {return new XExtCheckBox(n);}
static XComponent *XExtRadio_Creator(XmlNode *n) {return new XExtRadio(n);}
static XComponent *XExtIconButton_Creator(XmlNode *n) {return new XExtIconButton(n);}
static XComponent *XExtArrowButton_Creator(XmlNode *n) {return new XExtArrowButton(n);}

static XComponent *XExtPopup_Creator(XmlNode *n) {return new XExtPopup(n);}
static XComponent *XExtScrollBar_Hor_Creator(XmlNode *n) {return new XExtScrollBar(n, true);}
static XComponent *XExtScrollBar_Ver_Creator(XmlNode *n) {return new XExtScrollBar(n, false);}
static XComponent *XExtScroll_Creator(XmlNode *n) {return new XExtScroll(n);}
static XComponent *XExtTable_Creator(XmlNode *n) {return new XExtTable(n);}
static XComponent *XExtEdit_Creator(XmlNode *n) {return new XExtLineEdit(n);}
static XComponent *XExtTextArea_Creator(XmlNode *n) {return new XExtTextArea(n);}

static XComponent *XExtList_Creator(XmlNode *n) {return new XExtList(n);}
static XComponent *XExtComboBox_Creator(XmlNode *n) {return new XExtComboBox(n);}
static XComponent *XExtTree_Creator(XmlNode *n) {return new XExtTree(n);}
static XComponent *XExtCalendar_Creator(XmlNode *n) {return new XExtCalendar(n);}
static XComponent *XExtMaskEdit_Creator(XmlNode *n) {return new XExtMaskEdit(n);}
static XComponent *XExtPassword_Creator(XmlNode *n) {return new XExtPassword(n);}
static XComponent *XExtDatePicker_Creator(XmlNode *n) {return new XExtDatePicker(n);}

static XComponent *XExtWindow_Creator(XmlNode *n) {return new XExtWindow(n);}
static XComponent *XExtDialog_Creator(XmlNode *n) {return new XExtDialog(n);}

void UIFactory::init() {
	// INITCOMMONCONTROLSEX cc = {0};
	// cc.dwSize = sizeof(cc);
	// InitCommonControlsEx(&cc);
	UIFactory::registCreator("ExtEmpty", XExtEmptyComponent_Creator);
	UIFactory::registCreator("AbsLayout", XAbsLayout_Creator);
	UIFactory::registCreator("HLineLayout", XHLineLayout_Creator);
	UIFactory::registCreator("VLineLayout", XVLineLayout_Creator);
	UIFactory::registCreator("Window", XWindow_Creator);
	UIFactory::registCreator("Dialog", XDialog_Creator);

	UIFactory::registCreator("ExtButton", XExtButton_Creator);
	UIFactory::registCreator("ExtOption", XExtOption_Creator);
	UIFactory::registCreator("ExtLabel", XExtLabel_Creator);
	UIFactory::registCreator("ExtCheckBox", XExtCheckBox_Creator);
	UIFactory::registCreator("ExtRadio", XExtRadio_Creator);
	UIFactory::registCreator("ExtIconButton", XExtIconButton_Creator);
	UIFactory::registCreator("ExtLineEdit", XExtEdit_Creator);
	UIFactory::registCreator("ExtTextArea", XExtTextArea_Creator);

	UIFactory::registCreator("ExtPopup", XExtPopup_Creator);
	UIFactory::registCreator("ExtHorScrollBar", XExtScrollBar_Hor_Creator);
	UIFactory::registCreator("ExtVerScrollBar", XExtScrollBar_Ver_Creator);
	UIFactory::registCreator("ExtScroll", XExtScroll_Creator);
	UIFactory::registCreator("ExtTable", XExtTable_Creator);
	UIFactory::registCreator("ExtList", XExtList_Creator);
	UIFactory::registCreator("ArrowButton", XExtArrowButton_Creator);
	UIFactory::registCreator("ExtComboBox", XExtComboBox_Creator);
	UIFactory::registCreator("ExtTree", XExtTree_Creator);
	UIFactory::registCreator("ExtCalendar", XExtCalendar_Creator);
	UIFactory::registCreator("ExtMaskEdit", XExtMaskEdit_Creator);
	UIFactory::registCreator("ExtPassword", XExtPassword_Creator);
	UIFactory::registCreator("ExtDatePicker", XExtDatePicker_Creator);

	UIFactory::registCreator("ExtWindow", XExtWindow_Creator);
	UIFactory::registCreator("ExtDialog", XExtDialog_Creator);
}

//------------------------------------------------------

//----------------------------UIFactory-------------------------

static UIFactoryV::Creator g_creatorsV[64];
static int g_creatorNumV = 0;

VComponent* UIFactoryV::buildComponent( XmlNode *root) {
	if (root == NULL) {
		return NULL;
	}
	VComponent *x = create(root);
	root->setComponentV(x);
	for (int i = 0; i < root->getChildCount(); ++i) {
		XmlNode *child = root->getChild(i);
		VComponent *ix = buildComponent(child);
		child->setComponentV(ix);
	}
	return x;
}

XmlNode* UIFactoryV::buildNode( const char *resPath, const char *partName ) {
	return UIFactory::buildNode(resPath, partName);
}

VComponent* UIFactoryV::fastBuild( const char *resPath, const char *partName, VComponent *parent ) {
	XmlNode *root = buildNode(resPath, partName);
	if (root == NULL) {
		return NULL;
	}
	if (parent != NULL) {
		root->setParent(parent->getNode());
	}
	VComponent *cc = buildComponent(root);
	// cc->createWndTree();
	return cc;
}

void UIFactoryV::registCreator(Creator c ) {
	if (c == NULL)
		return;
	g_creatorsV[g_creatorNumV] = c;
	++g_creatorNumV;
}

void UIFactoryV::destory( XmlNode *root ) {
	for (int i = 0; i < root->getChildCount(); ++i) {
		XmlNode *child = root->getChild(i);
		destory(child);
	}
	delete root->getComponentV();
	delete root;
}

VComponent * UIFactoryV::create(XmlNode *node) {
	if (node == NULL) {
		return NULL;
	}
	if (node->getName() == NULL) {
		printf("UIFactoryV.create: node name is NULL \n");
		return NULL;
	}
	for (int i = 0; i < g_creatorNumV; ++i) {
		VComponent *find = g_creatorsV[i](node);
		if (find != NULL) {
			return find;
		}
	}
	printf("UIFactoryV.create: not find creator for '%s'\n", node->getName());
	return NULL;
}

static VComponent *UIFactoryV_Creator(XmlNode *n) {
	char *name = n->getName();
	if (strcmp(name, "Window") == 0) return new VWindow(n);
	if (strcmp(name, "Dialog") == 0) return new VDialog(n);
	if (strcmp(name, "Popup") == 0) return new VPopup(n);
	if (strcmp(name, "Label") == 0) return new VLabel(n);
	if (strcmp(name, "Button") == 0) return new VButton(n);
	if (strcmp(name, "Option") == 0) return new VOption(n);
	if (strcmp(name, "CheckBox") == 0) return new VCheckBox(n);
	if (strcmp(name, "Radio") == 0) return new VRadio(n);
	if (strcmp(name, "ScrollBar") == 0) return new VScrollBar(n);
	if (strcmp(name, "AbsLayout") == 0) return new VComponent(n);
	if (strcmp(name, "HLineLayout") == 0) return new VHLineLayout(n);
	if (strcmp(name, "VLineLayout") == 0) return new VVLineLayout(n);
	if (strcmp(name, "TextArea") == 0) return new VTextArea(n);
	return NULL;
}

void UIFactoryV::init() {
	registCreator(UIFactoryV_Creator);
}
