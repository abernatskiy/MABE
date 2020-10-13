class MABELogReader:
	'''Memory efficient MABE log reader'''
	def __init__(self, fileName):
		self.fileName = fileName

	def getAttributesList(self):
		with open(self.fileName, 'r') as logFile:
			firstLine = logFile.readline()
		return firstLine[:-1].split(',')

	def getAttributeTimeSeries(self, attrNames, dtype=float):
		'''Reads the columns from the log by column name.
		   Floats are read by default; pass a list of numpy
		   dtypes if you want something different.
		   Use ast.literal_eval to get the list-valued fields.
		'''
		#if dtypes and len(dtypes)!=len(attrNames):
		#	raise RuntimeError('Number of dtypes supplied ({}) differs from the number of attribute names ({})'.format(len(dtypes), len(attrNames)))
		attrList = self.getAttributesList()
		columns = [ attrList.index(attrName) for attrName in attrNames ]
		outColumns = [ [] for _ in range(len(attrNames)) ]
		with open(self.fileName, 'r') as logFile:
			logFile.readline()
			line = logFile.readline()
			while line:
				noQuotes = line[:-1].split('"')
				brokenLine = []
				for chunk in noQuotes:
					brokenLine += [chunk] if chunk[0]=='[' else chunk.split(',')
				brokenLine = [ s for s in brokenLine if s!='' ]

				columnVals = map(dtype, [ brokenLine[c] for c in columns ])
				for i,cv in enumerate(columnVals):
					outColumns[i].append(cv)
				line = logFile.readline()
		return outColumns
