#pragma once

#define CONCAT2(A, B) A ## B
#define CONCAT(A, B) CONCAT2(A, B)
#define UNIQUE_IDENT(Ident) CONCAT(Ident, __COUNTER__)