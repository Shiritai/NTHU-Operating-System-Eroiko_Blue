# Copyright (C) 2021 justin0u0<mail@justin0u0.com>

import click

@click.command()
@click.option('--output', default='./transformer.cpp', help='Output file path.')
@click.option('--answer', default='./tests/00_spec.json', help='Answer file path.')
def verify(output, answer):
	with open(output, 'r') as output_f, open(answer, 'r') as answer_f:
		output_lines = sorted(output_f.readlines())
		answer_lines = sorted(answer_f.readlines())

		for output_line, answer_line in zip(output_lines, answer_lines):
			if output_line != answer_line:
				print('\n\033[1;31;48m' + f'fail QAQ.' + '\033[1;37;0m')
				return

		print('\n\033[1;32;48m' + f'success ouo.' + '\033[1;37;0m')

if __name__ == '__main__':
	verify()
