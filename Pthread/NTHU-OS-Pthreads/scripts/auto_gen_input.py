# Copyright (C) 2021 justin0u0<mail@justin0u0.com>

import click
import json
import random

@click.command()
@click.option('--input', default='./tests/00_spec.json', help='Input json file path.')
@click.option('--output', default='./tests/00.out', help='Output file path.')
def generate(input, output):
	n = 0
	spec = {}

	with open(input, 'r') as jsonf:
		spec = json.load(jsonf)
		n = spec['n']
		spec = spec['auto_gen_input']

		print('\033[1;32;48m' + 'generating from spec ...' + '\033[1;37;0m')
		print('\033[1;34;48m' + f'n: {n}' + '\033[1;37;0m')
		print('\033[1;34;48m' + json.dumps(spec, indent=2) + '\033[1;37;0m')

	with open(output, 'w') as f:
		for i in range(n):
			key = i + 1
			val = random.randint(spec['low'], spec['high'])

			opcode = ''

			for c in spec['choices']:
				if i < int(c):
					opcode = random.choice(spec['choices'][c])
					break

			print(key, val, opcode, file=f)

	print('\n\033[1;32;48m' + f'done: [{output}].' + '\033[1;37;0m')

if __name__ == '__main__':
	generate()
