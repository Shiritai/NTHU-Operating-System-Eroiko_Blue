# Copyright (C) 2021 justin0u0<mail@justin0u0.com>

import click
import json

def generate_case(opcode, annotation, case_spec):
	template = f'''
	// {annotation}
	case '{opcode}':
		spec->a = {case_spec['a']};
		spec->b = {case_spec['b']};
		spec->m = {case_spec['m']};
		spec->iterations = {case_spec['iterations']};
		break;
'''
	
	return template

def generate_cpp(spec):
	producer_spec = ''
	for opcode in spec['annotation']:
		producer_spec += generate_case(opcode, spec['annotation'][opcode], spec['producer'][opcode])

	consumer_spec = ''
	for opcode in spec['annotation']:
		consumer_spec += generate_case(opcode, spec['annotation'][opcode], spec['consumer'][opcode])

	template = f'''// CODEGEN BY auto_gen_transformer.py; DO NOT EDIT.

#include <assert.h>
#include "transformer.hpp"

unsigned long long Transformer::producer_transform(char opcode, unsigned long long val) {{
	TransformSpec* spec = new TransformSpec;

	switch (opcode) {{{producer_spec}
	default:
		assert(false);
	}}

	return transform(spec, val);
}}

unsigned long long Transformer::consumer_transform(char opcode, unsigned long long val) {{
	TransformSpec* spec = new TransformSpec;

	switch (opcode) {{{consumer_spec}
	default:
		assert(false);
	}}

	return transform(spec, val);
}}

unsigned long long Transformer::transform(TransformSpec* spec, unsigned long long val) {{
	while (spec->iterations--) {{
		val = (val * spec->a + spec->b) % spec->m;
	}}
  return val;
}}
'''

	return template

@click.command()
@click.option('--input', default='./tests/00_spec.json', help='Input json file path.')
@click.option('--output', default='./transformer.cpp', help='Output .cpp file path.')
def generate(input, output):
	spec = {}

	with open(input, 'r') as jsonf:
		spec = json.load(jsonf)['auto_gen_transformer']

		print('\033[1;32;48m' + 'generating from spec ...' + '\033[1;37;0m')
		print('\033[1;34;48m' + json.dumps(spec, indent=2) + '\033[1;37;0m')

	with open(output, 'w') as f:
		print(generate_cpp(spec), file=f, end='')

	print('\n\033[1;32;48m' + f'done: [{output}].' + '\033[1;37;0m')

if __name__ == '__main__':
	generate()

