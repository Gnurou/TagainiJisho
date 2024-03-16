/*
 *  Copyright (C) 2008  Alexandre Courbot
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GUI_BOOKLETPRINTER_H_
#define __GUI_BOOKLETPRINTER_H_

#include <QPrinter>

#include "gui/BookletPrintEngine.h"

class BookletPrinter : public QPrinter {
  private:
    BookletPrintEngine _printEngine;

  public:
    BookletPrinter(QPrinter *printer, PrinterMode mode = ScreenResolution);
    virtual ~BookletPrinter();
};

#endif
