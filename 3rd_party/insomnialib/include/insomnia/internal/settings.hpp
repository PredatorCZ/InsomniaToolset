/*  InsomniaLib
    Copyright(C) 2021 Lukas Cone

    This program is free software : you can redistribute it and / or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#include "datas/internal/sc_architecture.hpp"

#ifdef IS_EXPORT
#define IS_EXTERN ES_EXPORT
#define IS_EXTERN_FN(what) ES_EXPORT_FN(what)
#elif defined(IS_IMPORT)
#define IS_EXTERN ES_IMPORT
#define IS_EXTERN_FN(what) ES_IMPORT_FN(what)
#else
#define IS_EXTERN
#define IS_EXTERN_FN(what) what
#endif
