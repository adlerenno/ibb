#include "stdlib.h"
#include "values.h"

typedef struct value {
    ssize_t start;
    ssize_t count;
    struct value *next;
} value;


Values New() {
    value *dummy = malloc(sizeof(value));
    dummy->start = -1;
    dummy->count = 0;
    dummy->next = NULL;
    Values v = {.data = dummy};
    return v;
}

void Destroy(Values v) {
    value *current = v.data;
    value *next = current->next;

    while (next != NULL) {
        free(current);
        current = next;
        next = next->next;
    }
    free(current);
}

static int cmp(const void *a, const void *b) {
    return (int) ((*(sequence *) a).pos - (*(sequence *) b).pos);
}

void add(Values v, sequence *data, size_t length) {

    qsort(data, length, sizeof(sequence), cmp);

    value *val = v.data;
    size_t i = 0;

    for (size_t j = 0; j < length; ++j) {
        while (val->next != NULL && val->next->start+val->next->count <= data[j].pos) {
            val = val->next;
            i += val->count;
        }

        data[j].rank = i;

        // Falls neuer Wert direkt hinter val Interval
        if (data[j].pos == val->start+val->count) {
            val->count++;
            i++;

            // Falls neuer Wert genau zwischen zwei values liegt,
            // vereine die beiden values
            if (val->next != NULL && val->start+val->count == val->next->start) { // falls neuer Wert genau zwischen
                val->count += val->next->count;
                i += val->next->count;
                value *n = val->next;
                val->next = n->next;
                free(n);
            }
        }
        // Falls neuer Wert direkt vor val liegt
        else if (val->next != NULL && val->next->start == data[j].pos+1) {
            val->next->count++;
            val->next->start--;
        } else {
            value *next = malloc(sizeof(value));
            next->start = (ssize_t) data[j].pos;
            next->count = 1;
            next->next = val->next;
            val->next = next;
        }

    }
}
