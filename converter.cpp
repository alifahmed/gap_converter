#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstring>
#include <cassert>
#include <random>

typedef int64_t i64;
typedef uint64_t u64;
typedef int32_t i32;
typedef uint32_t u32;

using namespace std;

typedef struct {
	u32 src;
	u32 dst;
} Edge;

void readSG(const char* __restrict name, u64 &numNodes, u64 &numEdges, vector<u64> &offsets, vector<u32> &neigh){
	FILE* in = fopen(name, "r");
	if(!in) {
		perror("Cannot open file.");
		exit(-1);
	}

	bool directed;
	u64 sz;

	sz = fread(&directed, 1, 1, in);
	//cout << sz << endl;
	assert(sz == 1);
	//assert(directed);

	sz = fread(&numEdges, 1, 8, in);
	//cout << sz << endl;
	assert(sz == 8);

	sz = fread(&numNodes, 1, 8, in);
	//cout << sz << endl;
	assert(sz == 8);
	
	//cout << "INFO: " << directed << " " << numEdges << " " << numNodes << endl;

	offsets.reserve(numNodes + 1);
	u64 offsetSize = (numNodes + 1) * 8;
	sz = fread(offsets.data(), 1, offsetSize, in);
	assert(sz == offsetSize);
//	cout << "Offsets:";
//	for(u32 i = 0; i < numNodes + 1; i++){
//		cout << " " << offsets[i];
//	}
//	cout << endl;

	neigh.reserve(numEdges);
	u64 neighSize = (numEdges) * 4;
	sz = fread(neigh.data(), 1, neighSize, in);
	assert(sz == neighSize);
//	cout << "Neighs:";
//	for(u32 i = 0; i < numEdges; i++){
//		cout << " " << neigh[i];
//	}
//	cout << endl;

	fclose(in);
}

void buildEL(u64 numNodes, u64 numEdges, vector<Edge> &el, const vector<u64> &offsets, const vector<u32> &neigh){
	el.resize(numEdges);

	#pragma omp parallel for
	for(u64 i = 0; i < numNodes; i++){
		u64 degree = offsets[i+1] - offsets[i];
		const u32* const __restrict readBase = neigh.data() + offsets[i];
		Edge* const __restrict writeBase = el.data() + offsets[i];
		for(u64 j = 0; j < degree; j++){
			u32 val = readBase[j];
			writeBase[j].src = i;
			writeBase[j].dst = val;
		}
	}
}

void writeELText(const char* __restrict name, u64 numEdges, const vector<Edge> &el){
	FILE* out = fopen(name, "w");
	if(!out) {
		perror("Cannot open file.");
		exit(-1);
	}
	char buffer[32];
	for(u64 i = 0; i < numEdges; i++){
		char* __restrict bufp = buffer + 32;
		*--bufp = '\n';
		u32 val = el[i].dst;
		do {
			*--bufp = (val % 10) + '0';
			val = val / 10;
		} while (val > 0);
		*--bufp = ' ';
		val = el[i].src;
		do {
			*--bufp = (val % 10) + '0';
			val = val / 10;
		} while (val > 0);
		//*bufp++ = '\n';

		fwrite_unlocked(bufp, 1, buffer + 32 - bufp, out);
	}
	fclose(out);
}

void writeELBin(const char* __restrict name, u64 numEdges, u64 numNodes, const vector<Edge> &el){
	FILE* out = fopen(name, "w");
	if(!out) {
		perror("Cannot open file.");
		exit(-1);
	}
	fwrite_unlocked(&numNodes, 8, 1, out);
	fwrite_unlocked(&numEdges, 8, 1, out);
	fwrite_unlocked(el.data(), sizeof(Edge), numEdges, out);
	fclose(out);
}


int main(int argc, const char** argv){
	if(argc != 3){
		cerr << "usage: ./converter <in.sg> <out.el>" << endl;
		return -1;
	}

	ios_base::sync_with_stdio(false);

	cout << "[" << argv[1] << "] Processing..." << endl;

	u64 numNodes, numEdges;
	vector<Edge> el;
	{
	vector<u64> offsets;
	vector<u32> neigh;
	cout << "[" << argv[1] << "] Reading offsets and neighbors..." << endl;
	readSG(argv[1], numNodes, numEdges, offsets, neigh);
	cout << "[" << argv[1] << "] Building edge list..." << endl;
	buildEL(numNodes, numEdges, el, offsets, neigh);
	}

	cout << "[" << argv[1] << "] Shuffling..." << endl;
	shuffle(el.begin(), el.end(), default_random_engine(42));
	//writeELText(argv[2], numEdges, el);

	cout << "[" << argv[1] << "] Writing to " << argv[2] << "..." << endl;
	writeELBin(argv[2], numEdges, numNodes, el);

	cout << "[" << argv[1] << "] Done" << endl;

//	for(u64 i = 0; i < numEdges; i++){
//		cout << el[i].src << " -> " << el[i].dst << endl;
//	}

	return 0;
}
