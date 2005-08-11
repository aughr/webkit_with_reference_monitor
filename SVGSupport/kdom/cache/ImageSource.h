/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
				  2004, 2005 Rob Buis <buis@kde.org>

    Based on khtml code by:
    Copyright (C) 1998 Lars Knoll <knoll@kde.org>
    Copyright (C) 2001-2003 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2003 Apple Computer, Inc

    This file is part of the KDE project

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef KDOM_ImageSource_H
#define KDOM_ImageSource_H

#ifdef APPLE_COMPILE_HACK
namespace KDOM
{
	class ImageSource;
}

#else

#include <qasyncio.h>

namespace KDOM
{
	class ImageSource : public QDataSource
	{
	public:
		ImageSource(QByteArray buf);
	
		void setEOF(bool state) { eof = state; }
	
		virtual int readyToSend();
		virtual void sendTo(QDataSink *sink, int n);

		virtual bool rewindable() const { return rewable; }
		virtual void enableRewind(bool on) { rew = on; }

		virtual void rewind();
		
		void cleanBuffer();
		
		QByteArray buffer;
		unsigned int pos;
		
	private:
		bool eof : 1;
		bool rew : 1;
		bool rewable : 1;
	};
};
#endif // APPLE_COMPILE_HACK

#endif

// vim:ts=4:noet
