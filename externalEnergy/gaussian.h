/*
 * gaussian.h
 *
 *  Created on: Dec 31, 2012
 *      Author: sethcall
 */

#ifndef GAUSSIAN_H_
#define GAUSSIAN_H_

#include "externalEnergy.h"
#include <algorithm>
#include <string>
#include <stdio.h>

class Gaussian : public ExternalEnergy {
public:
	static const char s_sPathToExecutable[];
	static const char s_sInputFileExtension[];
	static const char s_sOutputFileExtension[];
	static const char* s_optionalOutputFileExtensions[];

	Gaussian(const ExternalEnergyXml* pExternalEnergyXml);
	~Gaussian();

	bool createInputFile(const char* inputFileName, const Structure &structure);

	static bool readOutputFile(const char* outputFile, Structure &structure, bool readGeometry);

	bool setup() { return true; }
	bool execute(Structure &structure);
	bool clear() { return true; }

private:
	void searchForCheckPointFile();

	static bool s_bGetStandardOrientation;
	static bool s_bRequireNormalTermination;
	std::string m_sCheckPointFileName;

	bool isFileCharacter(char character);
};

#endif /* GAUSSIAN_H_ */
