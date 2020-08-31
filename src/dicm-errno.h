/*
 *  DICM, a library for reading DICOM instances
 *
 *  Copyright (c) 2020 Mathieu Malaterre
 *  All rights reserved.
 *
 *  DICM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, version 2.1.
 *
 *  DICM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with DICM . If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#define DICM_MAKE_ENUM_LIST

#ifdef DICM_MAKE_ENUM_LIST

typedef enum {

#define DICM_MESSAGE(code,string)   code ,

#endif /* DICM_MAKE_ENUM_LIST */

DICM_MESSAGE(kDicmInvalidArgument, "Invalid argument passed to the function" )
DICM_MESSAGE(kDicmOutOfOrder, "DataElement have been sent in out of order" )
DICM_MESSAGE(kDicmInvalidVR, "Value Representation if non-ASCII uppercase (A-Z only)" )

#ifdef DICM_MAKE_ENUM_LIST
   
  DICM_MSG_LASTMSGCODE
} DICM_MESSAGE_CODE;

#endif /* DICM_MAKE_ENUM_LIST */

#undef DICM_MAKE_ENUM_LIST
#undef DICM_MESSAGE
