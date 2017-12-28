#include "XText.h"
#include <stdlib.h>


XBaseText::XBaseText() : mText(NULL), mTextCapacity(0), mTextLen(0), 
	mHorAlign(ALIGN_LEFT), mVerAlign(ALIGN_VCENTER), mWidth(0), mHeight(0),
	mTextWidth(0), mTextHeight(0), mLineHeight(0) {
	
}
