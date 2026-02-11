#include "Annotation.h"

#include "FunctionArgProjection.h"

#include <stddef.h>

void Annotation_fillProjection(FunctionArgProjection* proj, Annotation* annotation) {
    Prototype* proto = annotation->project.proto;
        if (proto) {
            proj->name = annotation->project.name;
            proj->proto = proto;
        } else {
            proj->name = NULL;
            proj->tmpArgName = annotation->project.name;
        }
}
