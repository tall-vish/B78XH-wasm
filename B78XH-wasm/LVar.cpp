//    B78XH-wasm
//    Copyright (C) 2022  Heavy Division
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include "LVar.h"

LVar::LVar(PSTRINGZ id, PSTRINGZ unit): id(register_named_variable(id)),
                                        unit(get_units_enum(unit)) {
}

auto LVar::getValue() -> double {
	return this->value;
}

auto LVar::isValue() -> bool {
	return static_cast<bool>(this->value);
}

auto LVar::update() -> void {
	this->value = get_named_variable_typed_value(this->id, this->unit);
}
