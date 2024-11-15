#include "stdlib.h"
#include "values.h"

typedef struct value {
    ssize_t start;
    ssize_t count;
    struct value *next;
} value;


Values New(ssize_t length) {
    value *dummy = malloc(sizeof(value));
    dummy->start = -1;
    dummy->count = 0;
    dummy->next = NULL;
    return dummy;
}

void Destroy(Values v) {
    value *current = v;
    value *next = current->next;

    while (next != NULL) {
        free(current);
        current = next;
        next = next->next;
    }
    free(current);
}

void add(Values v, sequence *data, size_t length) {

    value *val = v;
    ssize_t i = 0;

    for (size_t j = 0; j < length; ++j) {
        ssize_t d = (ssize_t) data[j].pos;
        while (val->next != NULL && val->next->start + val->next->count <= d) {
            val = val->next;
            i += val->count;
        }

        data[j].rank = i;

        // Falls neuer Wert direkt hinter val Interval
        if (d == val->start + val->count) {
            val->count++;
            i++;

            // Falls neuer Wert genau zwischen zwei values liegt,
            // vereine die beiden values
            if (val->next != NULL && d + 1 == val->next->start) { // falls neuer Wert genau zwischen
                val->count += val->next->count;
                i += val->next->count;
                value *n = val->next;
                val->next = n->next;
                free(n);
            }
        }
        // Falls neuer Wert direkt vor val liegt
        else if (val->next != NULL && val->next->start == d + 1) {
            val->next->count++;
            val->next->start--;
        } else {
            value *next = malloc(sizeof(value));
            next->start = d;
            next->count = 1;
            next->next = val->next;
            val->next = next;
        }
    }
}
