#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
	FILE *in_fp, *out_fp;
	int file_size, stream_size, word_size, gen_len, code_len, word_cnt;
	char generator[10];
	char *code_stream, *code_array;
	int array_idx = 0;

	// usage error
	if (argc != 5){
		fprintf(stderr, "usage: ./crc_encoder input_file output_file generator dataword_size\n");
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

	// dataword size error
	word_size = atoi(argv[4]);
	if (word_size != 4 && word_size != 8) {
		fprintf(stderr, "dataword size must be 4 or 8.\n");
		return 1;
	}

	// generator setting
	memset(generator, 0, sizeof(generator));
	strcpy(generator, argv[3]);
	gen_len = strlen(generator);
	for (int i = 0; i < gen_len; i++) {
		generator[i] -= '0';
	}

	// stream size setting
	code_len = gen_len - 1 + word_size;
	word_cnt = file_size * 8 / word_size;
	stream_size = code_len * word_cnt / 8 + 1;
	if (code_len * word_cnt % 8)
		stream_size++;
	code_stream = (char *) malloc(sizeof(char) * stream_size);
	code_array = (char *) malloc(sizeof(char) * code_len * word_cnt);
	memset(code_stream, 0, stream_size);
	memset(code_array, 0, code_len * word_cnt);

	// input file read
	for (int r = 0; r < file_size; r++) {
		char input, data[8];

		memset(data, 0, 8);

		// 1byte read
		fread(&input, 1, 1, in_fp);

		for (int i = 0; i < 8; i++)
			data[7 - i] = (input >> i) & 1;

		// word size 4
		if (word_size == 4) {
			for (int i = 0; i <= 1; i++) {
				char *codeword;
				char divide[10];

				// initial codeword
				codeword = (char*)malloc(sizeof(char) * code_len);
				memset(codeword, 0, code_len);
				memset(divide, 0, 10);
				memcpy(codeword, data + (i * 4), 4);

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

				// code word calculate
				memcpy(codeword + 4, divide + 1, gen_len - 1);
				memcpy(code_array + array_idx, codeword, code_len);
				array_idx += code_len;	
				free(codeword);
			}
		}

		// data word size 8
		else {
			char *codeword;
			char divide[10];

			// initial codeword
			codeword = (char*)malloc(sizeof(char) * code_len);
			memset(codeword, 0, code_len);
			memset(divide, 0, 10);
			memcpy(codeword, data, 8);
			
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

			// code word calculate
			memcpy(codeword + 8, divide + 1, gen_len - 1);
			memcpy(code_array + array_idx, codeword, code_len);
			array_idx += code_len;
			free(codeword);
		}
	}




	// padding
	code_stream[0] = 8 - (code_len * word_cnt % 8);

	// save encoded code
	int code_idx = 0;
	unsigned char byte_num = 0;

	for (int j = 8 - (code_len * word_cnt % 8); j < 8; j++) {
		byte_num += code_array[code_idx];
		code_idx++;
		if (j != 7) byte_num <<= 1;
	}
	code_stream[1] = byte_num;

	for (int i = 2; i < stream_size; i++) {
		byte_num = 0;
		for (int j = 0; j < 8; j++) {
			byte_num += code_array[code_idx];
			code_idx++;
			if (j != 7) byte_num <<= 1;
		}
		code_stream[i] = byte_num;
	}

	// file write
	fwrite(code_stream, stream_size, 1, out_fp);

	// file close
	fclose(in_fp);
	fclose(out_fp);

	free(code_stream);
	free(code_array);

	return 0;
}
