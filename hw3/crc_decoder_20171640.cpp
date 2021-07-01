#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
	FILE *in_fp, *out_fp, *re_fp;
	int file_size, gen_len, word_size, code_len, code_cnt, err_cnt = 0, word_cnt;
	char generator[10], divide[10], *codeword;
	char *input_stream, *data_stream, *output_stream;
	int array_idx = 0, data_idx = 0, padding;

	// usage error
	if (argc != 6){
		fprintf(stderr, "usage: ./crc_decoder input_file output_file result_file generator dataword_size\n");
		exit(1);
	}

	// input file open
	in_fp = fopen(argv[1], "rb");
	if (in_fp == NULL){
		fprintf(stderr, "input file open error.\n");
		return 1;
	}

	// calculate file size
	fseek(in_fp, 0, SEEK_END);
	file_size = ftell(in_fp);

	// input file close
	fclose(in_fp);

	// input file reopen
	in_fp = fopen(argv[1], "rb");
	if (in_fp == NULL){
		fprintf(stderr, "input file open error.\n");
		return 1;
	}

	// output file open
	out_fp = fopen(argv[2], "wb");
	if (out_fp == NULL){
		fprintf(stderr, "output file open error.\n");
		return 1;
	}

	// result file open
	re_fp = fopen(argv[3], "w");
	if (re_fp == NULL){
		fprintf(stderr, "result file open error.\n");
		return 1;
	}

	// dataword size error
	word_size = atoi(argv[5]);
	if (word_size != 4 && word_size != 8) {
		fprintf(stderr, "dataword size must be 4 or 8.\n");
		return 1;
	}

	// generator setting
	memset(generator, 0, sizeof(generator));
	strcpy(generator, argv[4]);
	gen_len = strlen(generator);
	for (int i = 0; i < gen_len; i++) {
		generator[i] -= '0';
	}

	// codeword setting
	code_len = word_size + gen_len - 1;
	codeword = (char*)malloc(sizeof(char) * code_len);
	memset(codeword, 0, code_len);

	// read input file
	input_stream = (char*)malloc(sizeof(char) * file_size * 8);
	memset(input_stream, 0, file_size * 8);

	for (int i = 0; i < file_size; i++) {
		char input;
		fread(&input, 1, 1, in_fp);
		if (i == 0) padding = input;
		for (int j = 0; j < 8; j++) {
			input_stream[i * 8 + j] = (input >> (7 - j)) & 1;
		}
	}

	array_idx = padding + 8;
	code_cnt = (file_size * 8 - array_idx) / code_len;

	// data stream
	if (word_size == 4)
		word_cnt = code_cnt / 2;
	else
		word_cnt = code_cnt;

	data_stream = (char*)malloc(sizeof(char) * code_cnt * word_size);
	memset(data_stream, 0, code_cnt * word_size);

	// decode
	for (int r = 0; r < code_cnt; r++) {
		memset(codeword, 0, code_len);
		memcpy(codeword, input_stream + array_idx, code_len);
		array_idx += code_len;

		memset(divide, 0, 10);

		// division
		memcpy(divide, codeword, gen_len);
		for (int j = 0; j <= code_len - gen_len; j++) {
			if (divide[0]){
				for (int k = 0; k < gen_len; k++) {
					if (divide[k] == generator[k])
						divide[k] = 0;
					else
						divide[k] = 1;
				}
			}
			if (j != code_len - gen_len){
				memcpy(divide, divide + 1, gen_len - 1);
				divide[gen_len - 1] = codeword[j + gen_len];
			}
		}

		// error detect
		memcpy(divide, divide + 1, gen_len - 1);
		for (int j = 0; j < gen_len - 1; j++) {
			if (divide[j] != 0){
				err_cnt++;
				break;
			}
		}
		
		// data save
		for (int j = 0; j < word_size; j++) {
			data_stream[data_idx + j] = codeword[j];
		}
		data_idx += word_size;
	}

	output_stream = (char*)malloc(sizeof(char) * word_cnt);
	memset(output_stream, 0, word_cnt);

	int output_idx = 0;
	
	for (int i = 0; i < word_cnt * 8; i++) {
		output_stream[output_idx] += data_stream[i];
		if (i % 8 == 7) output_idx++;
		else output_stream[output_idx] <<= 1;
	}


	// file write
	fprintf(re_fp, "%d %d", code_cnt, err_cnt);
	fwrite(output_stream, output_idx, 1, out_fp);

	// file close
	fclose(in_fp);
	fclose(out_fp);
	fclose(re_fp);

	free(input_stream);
	free(codeword);
	free(data_stream);
	free(output_stream);

	return 0;
}
