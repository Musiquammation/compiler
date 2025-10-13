#include "primitives.h"
#include "globalLabelPool.h"

#include <string.h>
#include <stdlib.h>

PrimitiveContainer _primitives;

// comparaison par pointeur (pas strcmp)
static int cmpLabelPtr(const void* a, const void* b) {
    const label_t* la = (const label_t*)a;
    const label_t* lb = (const label_t*)b;
    if (*la < *lb) return -1;
    if (*la > *lb) return 1;
    return 0;
}

void primitives_init(void) {
    #define def(id, s, k) \
        _primitives.class_##id##s.definitionState = DEFINITIONSTATE_DONE; \
        _primitives.class_##id##s.name = _commonLabels._##id##s; \
        _primitives.class_##id##s.primitiveSizeCode = k; \
        _primitives.class_##id##s.size = s/8; \
        _primitives.class_##id##s.maxMinimalSize = s/8; \
        _primitives.type_##id##s.cl = &_primitives.class_##id##s; \
        _primitives.type_##id##s.primitiveSizeCode = k;\
        
    def(i, 8,  -1);
    def(u, 8,  1);
    def(i, 16, -2);
    def(u, 16, 2);
    def(i, 32, -4);
    def(u, 32, 4);
    def(i, 64, -8);
    def(u, 64, 8);
    def(f, 32, 126);
    def(f, 64, 127);

    #undef def

    struct {
        label_t lbl;
        Class*  cls;
        Type*   type;
    } tmp[] = {
        { _commonLabels._i8,    &_primitives.class_i8,    &_primitives.type_i8 },
        { _commonLabels._u8,    &_primitives.class_u8,    &_primitives.type_u8 },
        { _commonLabels._i16,   &_primitives.class_i16,   &_primitives.type_i16 },
        { _commonLabels._u16,   &_primitives.class_u16,   &_primitives.type_u16 },
        { _commonLabels._i32,   &_primitives.class_i32,   &_primitives.type_i32 },
        { _commonLabels._u32,   &_primitives.class_u32,   &_primitives.type_u32 },
        { _commonLabels._i64,   &_primitives.class_i64,   &_primitives.type_i64 },
        { _commonLabels._u64,   &_primitives.class_u64,   &_primitives.type_u64 },
        { _commonLabels._f32,   &_primitives.class_f32,   &_primitives.type_f32 },
        { _commonLabels._f64,   &_primitives.class_f64,   &_primitives.type_f64 },

        // alias
        { _commonLabels._char,  &_primitives.class_i8,    &_primitives.type_i8  },
        { _commonLabels._bool,  &_primitives.class_u8,    &_primitives.type_u8  },
        { _commonLabels._short, &_primitives.class_i16,   &_primitives.type_i16 },
        { _commonLabels._int,   &_primitives.class_i32,   &_primitives.type_i32 },
        { _commonLabels._long,  &_primitives.class_i64,   &_primitives.type_i64 },
        { _commonLabels._float, &_primitives.class_f32,   &_primitives.type_f32 },
        { _commonLabels._double,&_primitives.class_f64,   &_primitives.type_f64 },
    };

    enum {N = sizeof(tmp)/sizeof(tmp[0])};

    // extraire juste les labels pour trier
    label_t labels[N];
    for (int i = 0; i < N; i++)
        labels[i] = tmp[i].lbl;

    qsort(labels, N, sizeof(label_t), cmpLabelPtr);

    // reconstruire sortedLabels, sortedClasses, sortedTypes
    for (int i = 0; i < N; i++) {
        _primitives.sortedLabels[i] = labels[i];
        for (int j = 0; j < N; j++) {
            if (tmp[j].lbl == labels[i]) {
                _primitives.sortedClasses[i] = tmp[j].cls;
                _primitives.sortedTypes[i]   = tmp[j].type;
                break;
            }
        }
    }
}

Class* primitives_getClass(label_t name) {
    int low = 0, high = 17 - 1;
    while (low <= high) {
        int mid = (low + high) / 2;
        if (_primitives.sortedLabels[mid] == name)
            return _primitives.sortedClasses[mid];
        if (_primitives.sortedLabels[mid] < name)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return NULL;
}

Type* primitives_getType(Class* cl) {
    int low = 0, high = 17 - 1;
    while (low <= high) {
        int mid = (low + high) / 2;        
        if (_primitives.sortedClasses[mid] == cl)
            return _primitives.sortedTypes[mid];
        if (_primitives.sortedClasses[mid] < cl)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return NULL;
}
