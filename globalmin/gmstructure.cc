
#include "gmstructure.h"
#include "../rapidxml/rapidxml_utils.hpp"
#include "../rapidxml/rapidxml_print.hpp"

Structure::Structure() {
	m_iNumberOfAtomGroups = 0;
	m_atomGroups = NULL;
	m_atomGroupIndices = NULL;
	m_iNumberOfAtoms = 0;
	m_atomCoordinates = NULL;
	m_localAtomCoordinates = NULL;
	m_atomicNumbers = NULL;
	m_atomDistanceMatrix = NULL;
	m_atomGroupDistanceMatrix = NULL;
	m_fEnergy = 0;
	m_bIsTransitionState = false;
	m_id = 0;
	m_atomToCenterRanks = NULL;
}

Structure::~Structure() {
	clear();
}

void Structure::clear() {
	unsigned int i;

	m_sFilePrefix = "";

	if (m_atomDistanceMatrix != NULL) {
		for (i = 0; i < m_iNumberOfAtoms; ++i)
			delete[] m_atomDistanceMatrix[i];
		delete[] m_atomDistanceMatrix;
		m_atomDistanceMatrix = NULL;
	}

	m_iNumberOfAtoms = 0;
	if (m_atomCoordinates != NULL) {
		delete[] m_atomCoordinates;
		m_atomCoordinates = NULL;
	}
	if (m_localAtomCoordinates != NULL) {
		delete[] m_localAtomCoordinates;
		m_localAtomCoordinates = NULL;
	}
	if (m_atomicNumbers != NULL) {
		delete[] m_atomicNumbers;
		m_atomicNumbers = NULL;
	}

	if (m_atomGroupDistanceMatrix != NULL) {
		for (i = 0; i < m_iNumberOfAtomGroups; ++i)
			delete[] m_atomGroupDistanceMatrix[i];
		delete[] m_atomGroupDistanceMatrix;
		m_atomGroupDistanceMatrix = NULL;
	}

	m_iNumberOfAtomGroups = 0;
	if (m_atomGroups != NULL) {
		delete[] m_atomGroups;
		m_atomGroups = NULL;
	}

	if (m_atomGroupIndices != NULL) {
		delete[] m_atomGroupIndices;
		m_atomGroupIndices = NULL;
	}

	if (NULL != m_atomToCenterRanks) {
		delete[] m_atomToCenterRanks;
		m_atomToCenterRanks = NULL;
	}
}

const char* Structure::s_structureAttNames[] = {strings::xEnergy, strings::xIsTransitionState, strings::xId, strings::xFilePrefix};
const bool Structure::s_structureAttReq[] =    {true            ,true                        ,false        , false};
const char* Structure::s_structureAttDef[] =   {"0"             , strings::pFalse            , "0"         , NULL};

const unsigned int Structure::s_structureMinOccurs[] = {1};
const unsigned int Structure::s_structureMaxOccurs[] = {XSD_UNLIMITED};

bool Structure::load(const rapidxml::xml_node<>* pStructureElem) {
	using namespace rapidxml;
	using namespace strings;
	clear();

	XsdAttributeUtil structureAttUtil(s_structureAttNames, s_structureAttReq, s_structureAttDef);
	if (!structureAttUtil.process(pStructureElem)) {
		return false;
	}
	const char** structureAttValues = structureAttUtil.getAllAttributes();
	if (!XsdTypeUtil::getFloat(structureAttValues[0], m_fEnergy, s_structureAttNames[0], pStructureElem))
		return false;
	if (!XsdTypeUtil::getBoolValue(s_structureAttNames[1], structureAttValues[1], m_bIsTransitionState, pStructureElem))
		return false;
	if (!XsdTypeUtil::getInteger(structureAttValues[2], m_id, s_structureAttNames[2], pStructureElem))
		return false;
	if (NULL != structureAttValues[3])
		m_sFilePrefix = structureAttValues[3];

	const char* structureElemNames[] = {xAtomGroup};
	XsdElementUtil structureElemUtil(XSD_SEQUENCE, structureElemNames, s_structureMinOccurs, s_structureMaxOccurs);
	if (!structureElemUtil.process(pStructureElem)) {
		return false;
	}
	std::vector<const xml_node<>*>* structureElements = structureElemUtil.getSequenceElements();
	std::vector<const xml_node<>*>* atomGroups = &(structureElements[0]);
	m_iNumberOfAtomGroups = atomGroups->size();
	m_atomGroups = new AtomGroup[m_iNumberOfAtomGroups];
	m_iNumberOfAtoms = 0;
	for (unsigned int i = 0; i < m_iNumberOfAtomGroups; ++i) {
		if (!m_atomGroups[i].load((*atomGroups)[i]))
			return false;
		m_iNumberOfAtoms += m_atomGroups[i].getNumberOfAtoms();
	}
	initCoordinateRefs();
	update();

	return true;
}

bool Structure::loadStr(char* xml) {
	using namespace rapidxml;
	xml_document<> doc;
	try {
		doc.parse<0>(xml);
	} catch (const parse_error& e) {
		printf("XML parsing error: %s\n", e.what());
//		printf(e.where());
		return false;
	}
	const xml_node<>* pElem = doc.first_node();
	return load(pElem);
}

bool Structure::save(rapidxml::xml_document<> &doc, rapidxml::xml_node<>* pParentElem) const {
	rapidxml::xml_node<>* structure = save(doc);
	if (NULL == structure)
		return false;
	pParentElem->append_node(structure);
	return true;
}

bool Structure::save(std::string &buffer) const {
	using namespace rapidxml;
	xml_document<> doc;
	xml_node<>* structure = save(doc);
	if (NULL == structure)
		return false;
	doc.append_node(structure);
	rapidxml::print(std::back_inserter(buffer), doc, print_no_indenting);
	return true;
}

rapidxml::xml_node<>* Structure::save(rapidxml::xml_document<> &doc) const {
	using namespace rapidxml;
	using namespace strings;
	xml_node<>* structure = doc.allocate_node(node_element, xStructure);

	if (0 != m_fEnergy)
		XsdTypeUtil::setAttribute(doc, structure, xEnergy, m_fEnergy);
	if (m_bIsTransitionState)
		structure->append_attribute(doc.allocate_attribute(xIsTransitionState, pTrue, sizeof(xIsTransitionState)-1, sizeof(pTrue)-1));
	if (0 != m_id)
		XsdTypeUtil::setAttribute(doc, structure, xId, m_id);
	if (0 != m_sFilePrefix.length())
		structure->append_attribute(doc.allocate_attribute(xFilePrefix, m_sFilePrefix.c_str(), sizeof(xFilePrefix)-1, m_sFilePrefix.length()));

	for (unsigned int i = 0; i < m_iNumberOfAtomGroups; ++i)
		if (!m_atomGroups[i].save(doc, structure)) {
			delete structure;
			return NULL;
		}
	return structure;
}

void Structure::copy(const Structure &structure) {
	unsigned int i;
	clear();

	m_sFilePrefix = structure.m_sFilePrefix;
	m_iNumberOfAtomGroups = structure.m_iNumberOfAtomGroups;
	m_atomGroups = new AtomGroup[m_iNumberOfAtomGroups];
	for (unsigned int i = 0; i < m_iNumberOfAtomGroups; ++i)
		m_atomGroups[i].copy(structure.m_atomGroups[i]);

	m_iNumberOfAtoms = structure.m_iNumberOfAtoms;
	initCoordinateRefs();

	if (NULL != structure.m_atomGroupIndices) {
		memcpy(m_atomGroupIndices, structure.m_atomGroupIndices, sizeof(unsigned int) * m_iNumberOfAtomGroups);
	}
	if (NULL != structure.m_atomDistanceMatrix) {
		size_t nbytes = sizeof(FLOAT) * m_iNumberOfAtoms;
		for (i = 0; i < m_iNumberOfAtoms; ++i)
			memcpy(m_atomDistanceMatrix[i], structure.m_atomDistanceMatrix[i], nbytes);
	}
	if (NULL != structure.m_atomGroupDistanceMatrix) {
		size_t nbytes = sizeof(FLOAT) * m_iNumberOfAtomGroups;
		for (i = 0; i < m_iNumberOfAtomGroups; ++i)
			memcpy(m_atomGroupDistanceMatrix[i], structure.m_atomGroupDistanceMatrix[i], nbytes);
	}

	m_fEnergy = structure.m_fEnergy;
	m_bIsTransitionState = structure.m_bIsTransitionState;
	m_id = structure.m_id;

	if (NULL != structure.m_atomToCenterRanks) {
		m_atomToCenterRanks = new unsigned int[m_iNumberOfAtoms];
		memcpy(m_atomToCenterRanks, structure.m_atomToCenterRanks, sizeof(unsigned int) * m_iNumberOfAtoms);
	}
}

void Structure::setAtoms(unsigned int numAtoms,
		const COORDINATE4 *cartesianPoints, const unsigned int* atomicNumbers) {
	if (numAtoms != m_iNumberOfAtoms) {
		clear();
		m_iNumberOfAtoms = numAtoms;
		m_iNumberOfAtomGroups = 1;
		m_atomGroups = new AtomGroup[m_iNumberOfAtomGroups];
		m_atomGroups[0].setAtoms(numAtoms, cartesianPoints, atomicNumbers);
		initCoordinateRefs();
	} else {
		unsigned int i, j;
		j = 0;
		for (i = 0; i < m_iNumberOfAtomGroups; ++i) {
			m_atomGroups[i].setAtoms(m_atomGroups[i].getNumberOfAtoms(), &cartesianPoints[j], &atomicNumbers[j]);
			j += m_atomGroups[i].getNumberOfAtoms();
		}
	}
}

void Structure::setAtomGroups(unsigned int numAtomGroupTemplates, const AtomGroupTemplate* atomGroupTemplates) {
	unsigned int i, j, k, l;
	const AtomGroupTemplate* agtemp;
	const AtomGroup* ag;

	unsigned int iNumberOfAtomGroups = 0;
	unsigned int iNumberOfAtoms = 0;
	for (i = 0; i < numAtomGroupTemplates; ++i) {
		agtemp = &atomGroupTemplates[i];
		iNumberOfAtomGroups += agtemp->m_iNumber;
		iNumberOfAtoms += agtemp->m_iNumber * agtemp->m_atomicNumbers.size();
	}
	AtomGroup* atomGroups = new AtomGroup[iNumberOfAtomGroups];

	if (atomsMatch(numAtomGroupTemplates, atomGroupTemplates)) {
		// Assume this is a seeded structure and keep the same coordinates

		COORDINATE4 globalAtomCoordinates[m_iNumberOfAtoms];
		for (i = 0, j = 0; i < m_iNumberOfAtomGroups; ++i) {
			ag = &m_atomGroups[i];
			memcpy(&globalAtomCoordinates[j], m_atomCoordinates[j], SIZEOF_COORDINATE4 * ag->getNumberOfAtoms());
			j += ag->getNumberOfAtoms();
		}

		for (i = 0, k = 0, l = 0; i < numAtomGroupTemplates; ++i) {
			agtemp = &atomGroupTemplates[i];
			for (j = 1; j <= agtemp->m_iNumber; ++j) {
				atomGroups[k++].setAtoms(agtemp->m_atomicNumbers.size(), &globalAtomCoordinates[l], &m_atomicNumbers[l]);
				l += agtemp->m_atomicNumbers.size();
			}
		}
	} else {
		// Copy coordinates from the template
		for (i = 0, k = 0; i < numAtomGroupTemplates; ++i) {
			agtemp = &atomGroupTemplates[i];
			for (j = 1; j <= agtemp->m_iNumber; ++j)
				atomGroups[k++].setAtoms(*agtemp);
		}
	}

	clear();
	m_iNumberOfAtomGroups = iNumberOfAtomGroups;
	m_iNumberOfAtoms = iNumberOfAtoms;
	m_atomGroups = atomGroups;
	initCoordinateRefs();
}

bool Structure::atomsMatch(unsigned int numAtomGroupTemplates,
		const AtomGroupTemplate* atomGroupTemplates) {

	unsigned int groupi, groupj, groupn;
	const AtomGroupTemplate* templatei;
	unsigned int i;

	unsigned int totalAtoms = 0;
	unsigned int atomi = 0;
	for (groupi = 0; groupi < numAtomGroupTemplates; ++groupi) {
		templatei = &atomGroupTemplates[groupi];
		for (groupj = 0, groupn = templatei->m_iNumber; groupj < groupn; ++groupj) {
			totalAtoms += templatei->m_atomicNumbers.size();
			if (totalAtoms > m_iNumberOfAtoms)
				return false;
			for (i = 0; i < templatei->m_atomicNumbers.size(); ++i)
				if (m_atomicNumbers[atomi++] != templatei->m_atomicNumbers[i])
					return false;
		}
	}
	if (totalAtoms != m_iNumberOfAtoms)
		return false;
	return true;
}

void Structure::insertAtomGroup(AtomGroupTemplate &atomGroupTemplate, unsigned int index) {
	unsigned int iNumberOfAtomGroups = m_iNumberOfAtomGroups + 1;
	AtomGroup* atomGroups = new AtomGroup[iNumberOfAtomGroups];
	unsigned int iNumberOfAtoms = m_iNumberOfAtoms+atomGroupTemplate.m_atomicNumbers.size();

	unsigned int i;
	for (i = 0; i < index; ++i)
		atomGroups[i].copy(m_atomGroups[i]);
	atomGroups[index].setAtoms(atomGroupTemplate);
	for (i = index+1; i < iNumberOfAtomGroups; ++i)
		atomGroups[i].copy(m_atomGroups[i-1]);

	clear();
	m_iNumberOfAtomGroups = iNumberOfAtomGroups;
	m_iNumberOfAtoms = iNumberOfAtoms;
	m_atomGroups = atomGroups;
	initCoordinateRefs();
}

void Structure::deleteAtomGroup(unsigned int index) {
	unsigned int iNumberOfAtomGroups = m_iNumberOfAtomGroups - 1;
	AtomGroup* atomGroups = new AtomGroup[iNumberOfAtomGroups];
	unsigned int iNumberOfAtoms = m_iNumberOfAtoms - m_atomGroups[index].getNumberOfAtoms();

	unsigned int i;
	for (i = 0; i < index; ++i)
		atomGroups[i].copy(m_atomGroups[i]);
	for (i = index; i < iNumberOfAtomGroups; ++i)
		atomGroups[i].copy(m_atomGroups[i+1]);

	clear();
	m_iNumberOfAtomGroups = iNumberOfAtomGroups;
	m_iNumberOfAtoms = iNumberOfAtoms;
	m_atomGroups = atomGroups;
	initCoordinateRefs();
}

void Structure::initCoordinateRefs() {
	// Note: this method assumes the calling method has done the following before calling this method:
	// 1. Called the clear method
	// 2. Initialized m_iNumberOfAtoms, m_iNumberOfAtomGroups, and m_atomGroups
	// If this isn't the case, memory leaks may result.

	unsigned int i, j, k, n;
	m_atomCoordinates = new const COORDINATE4*[m_iNumberOfAtoms];
	m_localAtomCoordinates = new const COORDINATE4*[m_iNumberOfAtoms];
	m_atomicNumbers = new unsigned int[m_iNumberOfAtoms];
	const COORDINATE4* coordinates;
	const COORDINATE4* localCoordinates;
	k = 0;
	for (i = 0; i < m_iNumberOfAtomGroups; ++i) {
		memcpy(&m_atomicNumbers[k], m_atomGroups[i].getAtomicNumbers(), sizeof(unsigned int)*m_atomGroups[i].getNumberOfAtoms());
		coordinates = m_atomGroups[i].getGlobalAtomCoordinates();
		localCoordinates = m_atomGroups[i].getLocalAtomCoordinates();
		for (j = 0, n = m_atomGroups[i].getNumberOfAtoms(); j < n; ++j) {
			m_atomCoordinates[k] = &coordinates[j];
			m_localAtomCoordinates[k] = &localCoordinates[j];
			++k;
		}
	}

	m_atomDistanceMatrix = new FLOAT*[m_iNumberOfAtoms];
	for (i = 0; i < m_iNumberOfAtoms; ++i)
		m_atomDistanceMatrix[i] = new FLOAT[m_iNumberOfAtoms];

	m_atomGroupIndices = new unsigned int[m_iNumberOfAtomGroups];
	j = 0;
	for (i = 0; i < m_iNumberOfAtomGroups; ++i) {
		m_atomGroupIndices[i] = j;
		j += m_atomGroups[i].getNumberOfAtoms();
	}

	m_atomGroupDistanceMatrix = new FLOAT*[m_iNumberOfAtomGroups];
	for (i = 0; i < m_iNumberOfAtomGroups; ++i)
		m_atomGroupDistanceMatrix[i] = new FLOAT[m_iNumberOfAtomGroups];
}

void Structure::applyOperations() {
	for (unsigned int i = 0; i < m_iNumberOfAtomGroups; ++i) {
		m_atomGroups[i].initRotationMatrix();
		m_atomGroups[i].localToGlobal();
	}
}

void Structure::updateAtomDistanceMatrix() {
	unsigned int nminus1 = m_iNumberOfAtoms-1;
	unsigned int i, iAtom, jAtom;

	const FLOAT* coordinatei;
	const FLOAT* coordinatej;

	FLOAT distance, diff;
	for (iAtom = 0; iAtom < nminus1; ++iAtom) {
		coordinatei = *m_atomCoordinates[iAtom];
		for (jAtom = iAtom+1; jAtom < m_iNumberOfAtoms; ++jAtom)
		{
			coordinatej = *m_atomCoordinates[jAtom];

			distance = 0;
			for (i = 0; i < 3; ++i) {
				diff = coordinatei[i] - coordinatej[i];
				distance += diff*diff;
			}
			distance = sqrt(distance);
			m_atomDistanceMatrix[iAtom][jAtom] = distance;
			m_atomDistanceMatrix[jAtom][iAtom] = distance;
		}
	}
	for (iAtom = 0; iAtom < m_iNumberOfAtoms; ++iAtom)
		m_atomDistanceMatrix[iAtom][iAtom] = 0;
}

FLOAT Structure::findClosestDistance(unsigned int iAtomGroup1, unsigned int iAtomGroup2) {
	unsigned int iAtom1Start = m_atomGroupIndices[iAtomGroup1];
	unsigned int iAtom1End = iAtom1Start + m_atomGroups[iAtomGroup1].getNumberOfAtoms();
	unsigned int iAtom2Start = m_atomGroupIndices[iAtomGroup2];
	unsigned int iAtom2End = iAtom2Start + m_atomGroups[iAtomGroup2].getNumberOfAtoms();
	unsigned int iAtom1, iAtom2;

	FLOAT answer = 1e100; // some big number;
	FLOAT temp;
	for (iAtom1 = iAtom1Start; iAtom1 < iAtom1End; ++iAtom1)
		for (iAtom2 = iAtom2Start; iAtom2 < iAtom2End; ++iAtom2) {
			temp = m_atomDistanceMatrix[iAtom1][iAtom2];
			if (answer > temp)
				answer = temp;
		}

	return answer;
}

void Structure::updateAtomGroupDistanceMatrix() {
	unsigned int nminus1 = m_iNumberOfAtomGroups-1;
	unsigned int i, j;

	FLOAT distance;
	for (i = 0; i < nminus1; ++i)
		for (j = i+1; j < m_iNumberOfAtomGroups; ++j)
		{
			distance = findClosestDistance(i, j);
			m_atomGroupDistanceMatrix[i][j] = distance;
			m_atomGroupDistanceMatrix[j][i] = distance;
		}
	for (i = 0; i < m_iNumberOfAtomGroups; ++i)
		m_atomGroupDistanceMatrix[i][i] = 0;
}

unsigned int Structure::PRINT_RADIANS = 1;
unsigned int Structure::PRINT_ENERGY = 1 << 1;
unsigned int Structure::PRINT_LOCAL_COORDINATES = 1 << 2;
unsigned int Structure::PRINT_BOND_LENGTHS = 1 << 3;
unsigned int Structure::PRINT_DISTANCE_MATRIX = 1 << 4;

void Structure::print(unsigned int flags) const {
	const AtomGroup* atomGroup;
	unsigned int i, j, k;
	unsigned int n;
	COORDINATE3 diff;
	FLOAT lengthL, lengthG;
	if (flags & PRINT_ENERGY) {
		printf("Energy: %0.7lf\n", m_fEnergy);
	}
	for (i = 0; i < m_iNumberOfAtomGroups; ++i) {
		atomGroup = &m_atomGroups[i];
		atomGroup->print(flags & PRINT_RADIANS, flags & PRINT_LOCAL_COORDINATES);
		if (flags & PRINT_BOND_LENGTHS) {
			n = atomGroup->getNumberOfAtoms();
			for (j = 0; j < n; ++j) {
				for (k = j+1; k < n; ++k) {
					diff[0] = m_localAtomCoordinates[j][0] - m_localAtomCoordinates[k][0];
					diff[1] = m_localAtomCoordinates[j][1] - m_localAtomCoordinates[k][1];
					diff[2] = m_localAtomCoordinates[j][2] - m_localAtomCoordinates[k][2];
					lengthL = sqrt(diff[0]*diff[0] + diff[1]*diff[1] + diff[2]*diff[2]);
					diff[0] = m_atomCoordinates[j][0] - m_atomCoordinates[k][0];
					diff[1] = m_atomCoordinates[j][1] - m_atomCoordinates[k][1];
					diff[2] = m_atomCoordinates[j][2] - m_atomCoordinates[k][2];
					lengthG = sqrt(diff[0]*diff[0] + diff[1]*diff[1] + diff[2]*diff[2]);
					printf("Bond lengths between %u and %u, local = %0.7lf, global = %0.7lf\n", j+1, k+1, lengthL, lengthG);
				}
			}
		}
	}
	if (flags & PRINT_DISTANCE_MATRIX) {
		printDistanceMatrix(m_atomDistanceMatrix, m_atomicNumbers, m_iNumberOfAtoms);
	}
	printf("\n");
}

void Structure::printDistanceMatrix(const FLOAT* const * matrix,
		const unsigned int* atomicNumbers, unsigned int size) {
	unsigned int i, j;
	printf("Distance Matrix:\n");
	printf("   ");
	for (i = 0; i < size; ++i)
		printf("%7u:%2s", i+1, Handbook::getAtomicSymbol(atomicNumbers[i]));
	printf("\n");
	for (i = 0; i < size; ++i) {
		printf("%3u", i+1);
		for (j = 0; j < size; ++j) {
			printf("%10.4lf", matrix[i][j]);
		}
		printf("\n");
	}
}

void Structure::getCenterOfMass(COORDINATE3 &centerOfMass) {
	centerOfMass[0] = 0;
	centerOfMass[1] = 0;
	centerOfMass[2] = 0;
	FLOAT totalMass = 0;
	unsigned int i, j;
	const FLOAT* coordinate;

	for (i = 0; i < m_iNumberOfAtoms; ++i) {
		FLOAT mass = Handbook::getMass(m_atomicNumbers[i]);
		coordinate = *(m_atomCoordinates[i]);
		for (j = 0; j < 3; ++j)
			centerOfMass[j] += mass * coordinate[j];
		totalMass += mass;
	}
	for (j = 0; j < 3; ++j)
		centerOfMass[j] /= totalMass;
}

