// CODEGEN BY auto_gen_transformer.py; DO NOT EDIT.

#include <assert.h>
#include "transformer.hpp"

unsigned long long Transformer::producer_transform(char opcode, unsigned long long val) {
	TransformSpec* spec = new TransformSpec;

	switch (opcode) {
	// same speed
	case 'A':
		spec->a = 2003;
		spec->b = 183492;
		spec->m = 1000000007;
		spec->iterations = 9000000;
		break;

	// consumer faster than producer
	case 'B':
		spec->a = 2143;
		spec->b = 191324;
		spec->m = 1000000009;
		spec->iterations = 12000000;
		break;

	// producer faster than consumer
	case 'C':
		spec->a = 2089;
		spec->b = 923134;
		spec->m = 1000000021;
		spec->iterations = 5000000;
		break;

	// producer slightly faster than consumer
	case 'D':
		spec->a = 2677;
		spec->b = 912834;
		spec->m = 1000000033;
		spec->iterations = 7000000;
		break;

	// consumer slightly faster than producer
	case 'E':
		spec->a = 2693;
		spec->b = 718341;
		spec->m = 1000000087;
		spec->iterations = 12000000;
		break;

	default:
		assert(false);
	}

	return transform(spec, val);
}

unsigned long long Transformer::consumer_transform(char opcode, unsigned long long val) {
	TransformSpec* spec = new TransformSpec;

	switch (opcode) {
	// same speed
	case 'A':
		spec->a = 2729;
		spec->b = 713423;
		spec->m = 1000000093;
		spec->iterations = 9000000;
		break;

	// consumer faster than producer
	case 'B':
		spec->a = 2617;
		spec->b = 193424;
		spec->m = 1000000097;
		spec->iterations = 5000000;
		break;

	// producer faster than consumer
	case 'C':
		spec->a = 2053;
		spec->b = 743142;
		spec->m = 1000000103;
		spec->iterations = 12000000;
		break;

	// producer slightly faster than consumer
	case 'D':
		spec->a = 2347;
		spec->b = 617345;
		spec->m = 1000000123;
		spec->iterations = 12000000;
		break;

	// consumer slightly faster than producer
	case 'E':
		spec->a = 2521;
		spec->b = 4719832;
		spec->m = 1000000181;
		spec->iterations = 7000000;
		break;

	default:
		assert(false);
	}

	return transform(spec, val);
}

unsigned long long Transformer::transform(TransformSpec* spec, unsigned long long val) {
	while (spec->iterations--) {
		val = (val * spec->a + spec->b) % spec->m;
	}
  return val;
}
