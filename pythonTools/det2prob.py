#!/usr/bin/env python3

import argparse
import json
from pathlib import Path
from copy import deepcopy

def readIndivs(inPath):
	indivJsons = {}
	with open(inPath, 'r') as inFile:
		inFile.readline()
		for line in inFile:
			lineParts = line.split("'")
			indivID = int(lineParts[2][1:-1])
			indivJSON = json.loads(lineParts[1])
			indivJsons[indivID] = indivJSON
	return indivJsons

def binary2int(bits):
	out = 0
	for bit in reversed(bits):
		out = (out << 1) | bit
	return out

def probabilisticTable(deterministicTable):
	numPossibleOutputStates = 2**len(deterministicTable[0])
	return [ [ 1.0 if i==binary2int(outputState) else 0.0 for i in range(numPossibleOutputStates) ] for outputState in deterministicTable ]

def det2prob(inJson):
	outJson = []
	for layerJson in inJson:
		outLayerJson = deepcopy(layerJson)
		if layerJson['convolutionRegime']=='unshared':
			filters = layerJson['filters']
			filtersSizeX = len(filters)
			filtersSizeY = len(filters[0])
			filtersSizeT = len(filters[0][0])
			for x in range(filtersSizeX):
				for y in range(filtersSizeY):
					for t in range(filtersSizeT):
						for g in range(len(filters[x][y][t])):
							outLayerJson['filters'][x][y][t][g]['table'] = probabilisticTable(layerJson['filters'][x][y][t][g]['table'])
							outLayerJson['filters'][x][y][t][g]['type'] = 'ProbabilisticTexture'
		elif layerJson['convolutionRegime']=='shared':
			for g in range(len(layerJson['filters'])):
				outLayerJson['filters'][g]['table'] = probabilisticTable(layerJson['filters'][g]['table'])
				outLayerJson['filters'][g]['type'] = 'ProbabilisticTexture'
		outJson.append(outLayerJson)
	return outJson

def writeIndivs(indivJsons, outPath):
	if outPath.exists():
		raise ValueError(f'Output file {outPath} exists, will not overwrite')
	with open(outPath, 'w') as outFile:
		outFile.write('BRAIN_root::_json,ID\n')
		for id, idjson in indivJsons.items():
			jsonStr = json.dumps(idjson, separators=(',', ':'))
			outFile.write(f"'{jsonStr}',{id}\n")

if __name__=='__main__':
	parser = argparse.ArgumentParser(description='Convert saved DETexture brains from deterministic to probabilistic format')
	parser.add_argument('inFile', metavar='inFile', type=str)
	parser.add_argument('outFile', metavar='outFile', type=str)

	cliArgs = parser.parse_args()

	detIndivs = readIndivs(Path(cliArgs.inFile))
	probIndivs = {}
	for id, idjson in detIndivs.items():
		probIndivs[id] = det2prob(idjson)

	writeIndivs(probIndivs, Path(cliArgs.outFile))
