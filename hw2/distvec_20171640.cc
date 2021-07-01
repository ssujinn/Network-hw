#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INF 9999

typedef struct ROUTE_TAB {
	int dst;
	int next;
	int cost;
} ROUTE_TAB;

typedef struct MSG{
	int from;
	int to;
	char msg[1000];
}MSG;

typedef struct CHANGE{
	int node1;
	int node2;
	int cost;
}CHANGE;

int node_num;
int topology[100][100];

ROUTE_TAB routing_tab[100][100];

int msg_num;
MSG message_list[100];

int change_num;
CHANGE change_list[100];

int exchange_tab(){
	int change_flag = 0;

	// update i node routing table
	for (int i = 0; i < node_num; i++){
		// neighbor j
		for (int j = 0; j < node_num; j++){
			// neighbor: 자기자신이 아니고 direct link가 존재
			if (topology[i][j] != 0 && topology[i][j] != INF){
				// get neighbors' routing tab
				for (int k = 0; k < node_num; k++){
					// comparison my cost & neighbor's cost
					if (routing_tab[i][k].cost > topology[i][j] + routing_tab[j][k].cost){
						routing_tab[i][k].next = j;
						routing_tab[i][k].cost = topology[i][j] + routing_tab[j][k].cost;
						change_flag = 1;
					}
					// tie breaking rule
					else if (routing_tab[i][k].cost == topology[i][j] + routing_tab[j][k].cost){
						if (routing_tab[i][k].next > j){
							routing_tab[i][k].next = j;
							change_flag = 1;
						}
					}
				}
			}
		}
	}

	return change_flag;
}


void save_changes(FILE *fp){
	change_num = 0;
	int node1, node2, cost;

	while(!feof(fp)){
		fscanf(fp, "%d %d %d", &node1, &node2, &cost);
		change_list[change_num].node1 = node1;
		change_list[change_num].node2 = node2;
		change_list[change_num].cost = cost;
		change_num++;
	}

	change_num--;
}

void save_msg(FILE *fp){
	int cnt = 0;

	while (!feof(fp)){
		char input_message[1010];
		int from, to;
		char tmp[5];
		int blank_num = 0;
		int msg_idx = 0;

		memset(input_message, 0, sizeof(input_message));
		fgets(input_message, sizeof(input_message), fp);
		input_message[strlen(input_message) - 1] = 0;
		memset(message_list[cnt].msg, 0, 1000);

		for (int i = 0; i < (int)strlen(input_message); i++){
			if (input_message[i] == ' ' && blank_num == 0){
				memset(tmp, 0, sizeof(tmp));
				strncpy(tmp, input_message, i);
				from = atoi(tmp);
				message_list[cnt].from = from;
				msg_idx = i + 1;
				blank_num++;
			}
			else if (input_message[i] == ' ' && blank_num == 1){
				memset(tmp, 0, sizeof(tmp));
				strncpy(tmp, input_message + msg_idx, i - msg_idx);
				msg_idx = i + 1;
				to = atoi(tmp);
				message_list[cnt].to = to;
				blank_num++;
			}
			else if (blank_num == 2){
				strcpy(message_list[cnt].msg, input_message + msg_idx);
				break;
			}
		}
		cnt++;
	}

	msg_num = cnt - 1;
}


void print_tab(FILE *fp){
	for (int i = 0; i < node_num; i++){
		for (int j = 0; j < node_num; j++){
			if (routing_tab[i][j].cost < INF){
			fprintf(fp, "%d %d %d\n", routing_tab[i][j].dst, routing_tab[i][j].next, routing_tab[i][j].cost);
			}
		}
		fprintf(fp, "\n");
	}

}

void print_msg(FILE *fp){
	int src, dst;
	int hops[100];
	int hop_num = 0;

	for (int i = 0; i < msg_num; i++){
		hop_num = 0;
		if (routing_tab[message_list[i].from][message_list[i].to].cost < INF){
			src = message_list[i].from;
			dst = message_list[i].to;
			hops[hop_num] = src;
			hop_num++;

			while (routing_tab[src][dst].next != dst){
				src = routing_tab[src][dst].next;
				hops[hop_num] = src;
				hop_num++;
			}

			src = message_list[i].from;

			fprintf(fp, "from %d to %d cost %d hops ", message_list[i].from, message_list[i].to, routing_tab[src][dst].cost);
			for (int j = 0; j < hop_num; j++){
				fprintf(fp, "%d ", hops[j]);
			}
			fprintf(fp, "message %s\n", message_list[i].msg);
		}
		else {
			fprintf(fp, "from %d to %d cost infinite hops unreachable message %s\n", message_list[i].from, message_list[i].to, message_list[i].msg);
		}
	}
	fprintf(fp, "\n");

}

void init_tab(){
	for (int i = 0; i < node_num; i++){
		for (int j = 0; j < node_num; j++){
			routing_tab[i][j].dst = j;
			routing_tab[i][j].next = -1;	// no next
			routing_tab[i][j].cost = topology[i][j];

			if (topology[i][j] != 0 && topology[i][j] != INF)
				routing_tab[i][j].next = j;

			if (i == j)
				routing_tab[i][j].next = j;
		}
	}
}

int main(int argc, char *argv[]) {
	FILE *fp1, *fp2, *fp3, *out_fp;
	int node1, node2, cost;
	int converge = 0;

	// usage error
	if (argc != 4){
		fprintf(stderr, "usage: distvec topologyfile messagesfile changesfile\n");
		exit(1);
	}

	// file open
	fp1 = fopen(argv[1], "r");
	if (fp1 == NULL){
		fprintf(stderr, "Error: open input file.\n");
		return 1;
	}

	fp2 = fopen(argv[2], "r");
	if (fp2 == NULL){
		fprintf(stderr, "Error: open input file.\n");
		return 1;
	}

	fp3 = fopen(argv[3], "r");
	if (fp3 == NULL){
		fprintf(stderr, "Error: open input file.\n");
		return 1;
	}

	out_fp = fopen("output_dv.txt", "w");

	// save initial topology
	// no link -> INF
	memset(topology, INF, sizeof(topology));
	// save input cost
	fscanf(fp1, "%d", &node_num);
	while (EOF != fscanf(fp1, "%d %d %d", &node1, &node2, &cost)){
		topology[node1][node2] = cost;
		topology[node2][node1] = cost;
	}
	// i -> i cost 0
	for (int i = 0; i < node_num; i++)
		topology[i][i] = 0;

	// save message list
	save_msg(fp2);

	// save changes list
	save_changes(fp3);

	// routing table initialize
	init_tab();

	// exchange routing table
	while (converge != 10) {
		if (exchange_tab() == 0){
			converge++;
		}
	}

	// print first routing table
	print_tab(out_fp);

	// print message processing
	print_msg(out_fp);

	// change topology
	for (int i = 0; i < change_num; i++){
		node1 = change_list[i].node1;
		node2 = change_list[i].node2;
		cost = change_list[i].cost;
		topology[node1][node2] = cost;
		topology[node2][node1] = cost;

		// link disappear
		if (cost == -999){
			topology[node1][node2] = INF;
			topology[node2][node1] = INF;
		}

		init_tab();

		converge = 0;

		// exchange routing table
		while (converge != 10) {
			if (exchange_tab() == 0){
				converge++;
			}
		}

		// print routing table
		print_tab(out_fp);

		// print message processing
		print_msg(out_fp);
	}

	printf("Complete. Output file written to output_dv.txt.\n");

	fclose(out_fp);
	fclose(fp1);
	fclose(fp2);
	fclose(fp3);

	return 0;
}
