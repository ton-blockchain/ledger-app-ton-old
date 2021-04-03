#include "cell.h"
#include "utils.h"
#include "errors.h"

#include <stdio.h>

void Cell_init(struct Cell_t* self, uint8_t* cell_begin) {
    VALIDATE(self && cell_begin, ERR_CELL_IS_EMPTY);
    self->cell_begin = cell_begin;
}

uint8_t Cell_get_d1(const struct Cell_t* self) {
    VALIDATE(self && self->cell_begin, ERR_CELL_IS_EMPTY);
    return self->cell_begin[0];
}

uint8_t Cell_get_d2(const struct Cell_t* self) {
    VALIDATE(self && self->cell_begin, ERR_CELL_IS_EMPTY);
    return self->cell_begin[1];
}

uint8_t Cell_get_data_size(const struct Cell_t* self) {
    uint8_t d2 = Cell_get_d2(self);
    return (d2 >> 1) + (((d2 & 1) != 0) ? 1 : 0);
}

uint8_t* Cell_get_data(const struct Cell_t* self) {
    VALIDATE(self && self->cell_begin, ERR_CELL_IS_EMPTY);
    return self->cell_begin + CELL_DATA_OFFSET;
}

uint8_t* Cell_get_refs(const struct Cell_t* self, uint8_t* refs_count) {
    uint8_t d1 = Cell_get_d1(self);
    *refs_count = d1 & 7;
    uint8_t data_size = Cell_get_data_size(self);
    return self->cell_begin + CELL_DATA_OFFSET + data_size;
}
