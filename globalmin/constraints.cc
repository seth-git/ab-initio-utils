#include "constraints.h"
#include "gmstructure.h"

//const char*      Constraints::s_elementNames[]     = {"cube", "atomicDistances"};
const unsigned int Constraints::s_minOccurs[] = { 0, 0 };

//const char*      Constraints::s_distElementNames[] = {"min"        , "max"};
const unsigned int Constraints::s_distMinOccurs[] = { 0, 0 };
const unsigned int Constraints::s_distMaxOccurs[] = { XSD_UNLIMITED, 1 };

//const char* Constraints::s_attributeNames[] = {"name", "base"};
const bool    Constraints::s_required[]       = { true, false };
const char*   Constraints::s_defaultValues[]  = { "", "" };

//const char* Constraints::s_minAttributeNames[] = {"Value", "z1" , "z2"};
const bool    Constraints::s_minRequired[]       = { true, false, false };
const char*   Constraints::s_minDefaultValues[]  = { "", "", "" };

void Constraints::clear() {
	m_sName = "";
	m_pBase = NULL;
	if (m_pfCubeLWH != NULL) {
		delete m_pfCubeLWH;
		m_pfCubeLWH = NULL;
	}
	if (m_pfHalfCubeLWH != NULL) {
		delete m_pfHalfCubeLWH;
		m_pfHalfCubeLWH = NULL;
	}
	if (m_pfGeneralMinAtomicDistance != NULL) {
		delete m_pfGeneralMinAtomicDistance;
		m_pfGeneralMinAtomicDistance = NULL;
	}
	if (m_pfGeneralMaxAtomicDistance != NULL) {
		delete m_pfGeneralMaxAtomicDistance;
		m_pfGeneralMaxAtomicDistance = NULL;
	}
	if (m_rgMinAtomicDistances != NULL) {
		delete[] m_rgMinAtomicDistances;
		m_rgMinAtomicDistances = NULL;
		m_mapMinAtomicDistances.clear();
	}
}

void Constraints::setCubeLWH(FLOAT cubeLWH) {
	if (NULL == m_pfCubeLWH) {
		m_pfCubeLWH = new FLOAT;
		m_pfHalfCubeLWH = new FLOAT;
	}
	*m_pfCubeLWH = cubeLWH;
	*m_pfHalfCubeLWH = cubeLWH * 0.5;
}

bool Constraints::load(const rapidxml::xml_node<>* pConstraintsElem,
                       std::map<std::string, Constraints*> &constraintsMap) {
	using namespace rapidxml;
	using namespace strings;
	const char* elementNames[] = { xCube, xAtomicDistances };
	const char* distElementNames[] = { xMin, xMax };
	XsdElementUtil constraintUtil(XSD_ALL, elementNames, s_minOccurs);

	clear();

	const char* valueAttNames[] = { xName, xBase };
	const char** values;
	XsdAttributeUtil valueUtil(valueAttNames, s_required, s_defaultValues);
	if (!valueUtil.process(pConstraintsElem))
		return false;
	values = valueUtil.getAllAttributes();

	if (values[1] != NULL) {
		Constraints* other = constraintsMap[values[1]];
		if (other == NULL) {
			printf(ConstraintNameMisMatch, pConstraintsElem->name(), valueAttNames[0], values[0], xBase, values[1]);
			return false;
		}
		copy(*other);
		m_pBase = other;
	}
	m_sName = values[0];

	if (!constraintUtil.process(pConstraintsElem))
		return false;
	const xml_node<>** constraintElements = constraintUtil.getAllElements();

	if (constraintElements[0] != NULL) {
		if (!m_pfCubeLWH) {
			m_pfCubeLWH = new FLOAT;
			m_pfHalfCubeLWH = new FLOAT;
		}
		if (!XsdTypeUtil::read1PosFloatAtt(constraintElements[0], *m_pfCubeLWH, xSize, true, NULL))
			return false;
		*m_pfHalfCubeLWH = *m_pfCubeLWH * 0.5;
	}

	if (constraintElements[1] != NULL) {
		XsdElementUtil distUtil(XSD_SEQUENCE, distElementNames, s_distMinOccurs, s_distMaxOccurs);
		std::vector<const xml_node<>*>* distElements;
		unsigned int i;

		if (!distUtil.process(constraintElements[1]))
			return false;
		distElements = distUtil.getSequenceElements();
		unsigned int timesReadGeneralMin = 0;
		for (i = 0; i < distElements[0].size(); ++i)
			if (!addMinDist(distElements[0][i], timesReadGeneralMin))
				return false;
		for (i = 0; i < distElements[1].size(); ++i) { // Should be only one
			if (!m_pfGeneralMaxAtomicDistance)
				m_pfGeneralMaxAtomicDistance = new FLOAT;
			if (!XsdTypeUtil::read1PosFloatAtt(distElements[1][i], *m_pfGeneralMaxAtomicDistance, xValue, true, NULL)) {
				return false;
			}
		}
	}

	return true;
}

bool Constraints::addMinDist(const rapidxml::xml_node<>* pElem, unsigned int &timesReadGeneralMin) {
	using namespace strings;
	const char* minAttributeNames[] = {xValue, xZ1, xZ2 };
	const char** values;
	XsdAttributeUtil attUtil(minAttributeNames, s_minRequired, s_minDefaultValues);
	if (!attUtil.process(pElem))
		return false;
	values = attUtil.getAllAttributes();

	if ((values[1] == NULL && values[2] != NULL) || (values[1] != NULL && values[2] == NULL)) {
		printf(ErrorZ1Z2, pElem->name(), minAttributeNames[1], minAttributeNames[2]);
		return false;
	}

	if (values[1] == NULL) {
		if (timesReadGeneralMin >= 1) {
			printf(ErrorOneGeneralMin, pElem->name(), minAttributeNames[1], minAttributeNames[2]);
			return false;
		}
		if (!m_pfGeneralMinAtomicDistance)
			m_pfGeneralMinAtomicDistance = new FLOAT;
		XsdTypeUtil::getPositiveFloat(values[0], *m_pfGeneralMinAtomicDistance, minAttributeNames[0], pElem);
		++timesReadGeneralMin;
	} else {
		unsigned int i, j;
		if (m_rgMinAtomicDistances == NULL) {
			m_rgMinAtomicDistances = new MinDistArray[MIN_DIST_ARRAY_SIZE];
			for (i = 1; i <= MAX_ATOMIC_NUMBERS; ++i)
				for (j = 1; j <= MAX_ATOMIC_NUMBERS; ++j)
					m_rgMinAtomicDistances[i][j] = -1;
		}
		if (!XsdTypeUtil::getAtomicNumber(values[1], i, minAttributeNames[1], pElem->name()))
			return false;
		if (!XsdTypeUtil::getAtomicNumber(values[2], j, minAttributeNames[2], pElem->name()))
			return false;
		if (m_rgMinAtomicDistances[i][j] != -1) {
			printf(ErrorDuplicateMinDist, pElem->name(), minAttributeNames[1], i, minAttributeNames[2], j);
			return false;
		}
		if (!XsdTypeUtil::getPositiveFloat(values[0], m_rgMinAtomicDistances[i][j], minAttributeNames[0], pElem))
			return false;
		m_rgMinAtomicDistances[j][i] = m_rgMinAtomicDistances[i][j];
		m_mapMinAtomicDistances[i][j] = m_rgMinAtomicDistances[i][j];
	}
	return true;
}

bool Constraints::save(rapidxml::xml_document<> &doc, rapidxml::xml_node<>* pConstraintsElem) const {
	using namespace rapidxml;
	using namespace strings;
	pConstraintsElem->append_attribute(doc.allocate_attribute(xName, m_sName.c_str()));
	if (m_pBase)
		pConstraintsElem->append_attribute(doc.allocate_attribute(xBase, m_pBase->m_sName.c_str()));
	if (m_pfCubeLWH && (!m_pBase || !m_pBase->m_pfCubeLWH || *m_pfCubeLWH != *(m_pBase->m_pfCubeLWH))) {
		xml_node<>* cube = doc.allocate_node(node_element, xCube);
		XsdTypeUtil::setAttribute(doc, cube, xSize, *m_pfCubeLWH);
		pConstraintsElem->append_node(cube);
	}

	bool writeGeneralMin = m_pfGeneralMinAtomicDistance && (!m_pBase || !m_pBase->m_pfGeneralMinAtomicDistance || *m_pfGeneralMinAtomicDistance != *(m_pBase->m_pfGeneralMinAtomicDistance));
	bool writeGeneralMax = m_pfGeneralMaxAtomicDistance && (!m_pBase || !m_pBase->m_pfGeneralMaxAtomicDistance || *m_pfGeneralMaxAtomicDistance != *(m_pBase->m_pfGeneralMaxAtomicDistance));
	bool writeSpecificMins = specificMinDistNotInBase();

	if (writeGeneralMin || writeGeneralMax || writeSpecificMins) {
		xml_node<>* atomicDistances = doc.allocate_node(node_element, xAtomicDistances);
		if (writeGeneralMin) {
			xml_node<>* generalMin = doc.allocate_node(node_element, xMin);
			XsdTypeUtil::setAttribute(doc, generalMin, xValue, *m_pfGeneralMinAtomicDistance);
			atomicDistances->append_node(generalMin);
		}
		if (writeSpecificMins) {
			xml_node<>* min;
			for (std::map<unsigned int, std::map<unsigned int, FLOAT> >::const_iterator i = m_mapMinAtomicDistances.begin(); i != m_mapMinAtomicDistances.end(); i++)
				for (std::map<unsigned int, FLOAT>::const_iterator j = i->second.begin(); j != i->second.end(); j++)
					if (!m_pBase || j->second != m_pBase->m_rgMinAtomicDistances[i->first][j->first]) {
						min = doc.allocate_node(node_element, xMin);
						XsdTypeUtil::setAttribute(doc, min, xValue, j->second);
						XsdTypeUtil::setAttribute(doc, min, xZ1, i->first);
						XsdTypeUtil::setAttribute(doc, min, xZ2, j->first);
						atomicDistances->append_node(min);
					}
		}
		if (writeGeneralMax) {
			xml_node<>* generalMax = doc.allocate_node(node_element, xMax);
			XsdTypeUtil::setAttribute(doc, generalMax, xValue, *m_pfGeneralMaxAtomicDistance);
			atomicDistances->append_node(generalMax);
		}
		pConstraintsElem->append_node(atomicDistances);
	}
	return true;
}

bool Constraints::specificMinDistNotInBase() const {
	if (!m_rgMinAtomicDistances)
		return false;

	if (!m_pBase || !m_pBase->m_rgMinAtomicDistances)
		return true;

	for (std::map<unsigned int, std::map<unsigned int, FLOAT> >::const_iterator i = m_mapMinAtomicDistances.begin(); i != m_mapMinAtomicDistances.end(); i++)
		for (std::map<unsigned int, FLOAT>::const_iterator j = i->second.begin(); j != i->second.end(); j++)
			if (j->second != m_pBase->m_rgMinAtomicDistances[i->first][j->first])
				return true;

	return false;
}

void Constraints::copy(const Constraints &other) {
	clear();
	m_sName = other.m_sName;
	m_pBase = other.m_pBase;
	if (other.m_pfCubeLWH != NULL) {
		m_pfCubeLWH = new FLOAT;
		*m_pfCubeLWH = *other.m_pfCubeLWH;
		m_pfHalfCubeLWH = new FLOAT;
		*m_pfHalfCubeLWH = *other.m_pfHalfCubeLWH;
	}
	
	if (other.m_pfGeneralMinAtomicDistance != NULL) {
		m_pfGeneralMinAtomicDistance = new FLOAT;
		*m_pfGeneralMinAtomicDistance = *other.m_pfGeneralMinAtomicDistance;
	}
	
	if (other.m_pfGeneralMaxAtomicDistance != NULL) {
		m_pfGeneralMaxAtomicDistance = new FLOAT;
		*m_pfGeneralMaxAtomicDistance = *other.m_pfGeneralMaxAtomicDistance;
	}
	
	if (other.m_rgMinAtomicDistances != NULL) {
		m_rgMinAtomicDistances = new MinDistArray[MIN_DIST_ARRAY_SIZE];
		memcpy(m_rgMinAtomicDistances, other.m_rgMinAtomicDistances, SIZEOF_MIN_DIST_ARRAY);

		for (std::map<unsigned int, std::map<unsigned int, FLOAT> >::const_iterator i = other.m_mapMinAtomicDistances.begin(); i != other.m_mapMinAtomicDistances.end(); i++)
			for (std::map<unsigned int, FLOAT>::const_iterator j = i->second.begin(); j != i->second.end(); j++)
				m_mapMinAtomicDistances[i->first][j->first] = j->second;
	}
}

bool Constraints::validate(Structure &structure) const {
	if (!ensureInsideContainer(structure))
		return false;
	if (!minDistancesOK(structure))
		return false;
	if (!maxDistancesOK(structure))
		return false;
	return true;
}

FLOAT Constraints::getSmallestMinDistance() const {
	const static FLOAT start = 1000.0;
	FLOAT smallestMin = start;
	if (NULL != m_pfGeneralMinAtomicDistance)
		smallestMin = *m_pfGeneralMinAtomicDistance;
	FLOAT min;
	for (std::map<unsigned int, std::map<unsigned int, FLOAT> >::const_iterator i = m_mapMinAtomicDistances.begin(); i != m_mapMinAtomicDistances.end(); i++)
		for (std::map<unsigned int, FLOAT>::const_iterator j = i->second.begin(); j != i->second.end(); j++) {
			min = j->second;
			if (min < smallestMin)
				smallestMin = min;
		}
	if (smallestMin == start)
		return 0;
	else
		return smallestMin;
}

void Constraints::combineConstraints(const Constraints &other) {
	if (NULL != other.m_pfCubeLWH) {
		if (NULL == m_pfCubeLWH) {
			m_pfCubeLWH = new FLOAT;
			m_pfHalfCubeLWH = new FLOAT;
			*m_pfCubeLWH = *other.m_pfCubeLWH;
			*m_pfHalfCubeLWH = *other.m_pfHalfCubeLWH;
		} else {
			if (*other.m_pfCubeLWH < *m_pfCubeLWH) {
				*m_pfCubeLWH = *other.m_pfCubeLWH;
				*m_pfHalfCubeLWH = *other.m_pfHalfCubeLWH;
			}
		}
	}
	if (NULL != other.m_pfGeneralMinAtomicDistance) {
		if (NULL == m_pfGeneralMinAtomicDistance) {
			m_pfGeneralMinAtomicDistance = new FLOAT;
			*m_pfGeneralMinAtomicDistance = *other.m_pfGeneralMinAtomicDistance;
		} else {
			if (*other.m_pfGeneralMinAtomicDistance < *m_pfGeneralMinAtomicDistance)
				*m_pfGeneralMinAtomicDistance = *other.m_pfGeneralMinAtomicDistance;
		}
	}
	if (NULL != other.m_pfGeneralMaxAtomicDistance) {
		if (NULL == m_pfGeneralMaxAtomicDistance) {
			m_pfGeneralMaxAtomicDistance = new FLOAT;
			*m_pfGeneralMaxAtomicDistance = *other.m_pfGeneralMaxAtomicDistance;
		} else {
			if (*other.m_pfGeneralMaxAtomicDistance < *m_pfGeneralMaxAtomicDistance)
				*m_pfGeneralMaxAtomicDistance = *other.m_pfGeneralMaxAtomicDistance;
		}
	}

	if (NULL != other.m_rgMinAtomicDistances) {
		if (NULL == m_rgMinAtomicDistances) {
			m_rgMinAtomicDistances = new MinDistArray[MIN_DIST_ARRAY_SIZE];
			memcpy(m_rgMinAtomicDistances, other.m_rgMinAtomicDistances, SIZEOF_MIN_DIST_ARRAY);

			for (std::map<unsigned int, std::map<unsigned int, FLOAT> >::const_iterator i = other.m_mapMinAtomicDistances.begin(); i != other.m_mapMinAtomicDistances.end(); i++)
				for (std::map<unsigned int, FLOAT>::const_iterator j = i->second.begin(); j != i->second.end(); j++)
					m_mapMinAtomicDistances[i->first][j->first] = j->second;
		} else {
			for (std::map<unsigned int, std::map<unsigned int, FLOAT> >::const_iterator i = other.m_mapMinAtomicDistances.begin(); i != other.m_mapMinAtomicDistances.end(); i++)
				for (std::map<unsigned int, FLOAT>::const_iterator j = i->second.begin(); j != i->second.end(); j++)
					if (j->second < m_rgMinAtomicDistances[i->first][j->first]) {
						m_rgMinAtomicDistances[i->first][j->first] = j->second;
						m_mapMinAtomicDistances[i->first][j->first] = j->second;
					}
		}
	}
}

FLOAT Constraints::getMinDistance(unsigned int atomicNumber1, unsigned int atomicNumber2) const {
	FLOAT dist = -1;
	if (NULL != m_rgMinAtomicDistances)
		dist = m_rgMinAtomicDistances[atomicNumber1][atomicNumber2];
	if (-1 == dist) {
		if (NULL != m_pfGeneralMinAtomicDistance)
			dist = *m_pfGeneralMinAtomicDistance;
		else
			dist = 0;
	}
	return dist;
}

bool Constraints::minDistancesOK(
		const std::map<unsigned int, bool> &atomGroupsInitialized,
		const Structure &structure) const {
	const FLOAT* const* distanceMatrix = structure.getAtomDistanceMatrix();
	const unsigned int* atomicNumbers = structure.getAtomicNumbers();
	const AtomGroup* atomGroups = structure.getAtomGroups();
	unsigned int n = structure.getNumberOfAtomGroups();
	unsigned int iGroup;

	std::map<unsigned int, bool> atomsInitialized;
	unsigned int atomi = 0;
	unsigned int atomiEnd;
	for (iGroup = 0; iGroup < n; ++iGroup) {
		atomiEnd = atomi + atomGroups[iGroup].getNumberOfAtoms();
		if (atomGroupsInitialized.find(iGroup) == atomGroupsInitialized.end()) {
			atomi = atomiEnd;
			continue;
		}
		do {
			atomsInitialized[atomi++] = true;
		} while (atomi < atomiEnd);
	}

	std::map<unsigned int, bool>::iterator i, j;

	for (i = atomsInitialized.begin(); i != atomsInitialized.end(); i++)
		for (j = i, j++; j != atomsInitialized.end(); j++) {
			if (distanceMatrix[i->first][j->first] < getMinDistance(atomicNumbers[i->first], atomicNumbers[j->first])) {
				return false;
			}
		}
	return true;
}

bool Constraints::minDistancesOK(const Structure &structure) const {
	const FLOAT* const* distanceMatrix = structure.getAtomDistanceMatrix();
	const unsigned int* atomicNumbers = structure.getAtomicNumbers();
	unsigned int n = structure.getNumberOfAtoms();
	unsigned int nminus1 = n - 1;
	unsigned int i, j;

	for (i = 0; i < nminus1; i++)
		for (j = i+1; j < n; j++)
			if (distanceMatrix[i][j] < getMinDistance(atomicNumbers[i], atomicNumbers[j]))
				return false;

	return true;
}

bool Constraints::maxDistancesOK(
		const std::map<unsigned int, bool> &atomGroupsInitialized,
		const Structure &structure) const {
	if (NULL == m_pfGeneralMaxAtomicDistance)
		return true;
	unsigned int n = structure.getNumberOfAtomGroups();
	const FLOAT* const* distanceMatrix = structure.getAtomGroupDistanceMatrix();
	unsigned int i, j;

	// Make an adjacency matrix
	unsigned int i2, j2;
	unsigned int n2 = atomGroupsInitialized.size();
	bool** adjacencyMatrix = new bool*[n2];
	for (i2 = 0; i2 < n2; ++i2)
		adjacencyMatrix[i2] = new bool[n2];
	bool temp;
	unsigned int nminus1 = n-1;
	for (i = 0, i2 = 0; i < nminus1; ++i) {
		if (atomGroupsInitialized.find(i) == atomGroupsInitialized.end())
			continue;
		for (j = i+1, j2 = i2+1; j < n; ++j) {
			if (atomGroupsInitialized.find(j) == atomGroupsInitialized.end())
				continue;
			temp = distanceMatrix[i][j] <= *m_pfGeneralMaxAtomicDistance;
			adjacencyMatrix[i2][j2] = temp;
			adjacencyMatrix[j2][i2] = temp;
			++j2;
		}
		++i2;
	}
	for (i2 = 0; i2 < n2; ++i2)
		adjacencyMatrix[i2][i2] = false;

	unsigned int numberVisited = 0;
	bool visited[n2];
	for (i2 = 0; i2 < n2; ++i2)
		visited[i2] = false;
	depthFirstSearch(0, numberVisited, visited, adjacencyMatrix, n2);

	for (i2 = 0; i2 < n2; ++i2)
		delete[] adjacencyMatrix[i2];
	delete[] adjacencyMatrix;

	return numberVisited == n2;
}

bool Constraints::maxDistancesOK(const Structure &structure) const {
	if (NULL == m_pfGeneralMaxAtomicDistance)
		return true;
	unsigned int n = structure.getNumberOfAtomGroups();
	const FLOAT* const* distanceMatrix = structure.getAtomGroupDistanceMatrix();
	unsigned int i, j;

	// Make an adjacency matrix
	bool** adjacencyMatrix = new bool*[n];
	for (i = 0; i < n; ++i)
		adjacencyMatrix[i] = new bool[n];
	bool temp;
	unsigned int nminus1 = n-1;
	for (i = 0; i < nminus1; ++i)
		for (j = i+1; j < n; ++j) {
			temp = distanceMatrix[i][j] <= *m_pfGeneralMaxAtomicDistance;
			adjacencyMatrix[i][j] = temp;
			adjacencyMatrix[j][i] = temp;
		}
	for (i = 0; i < n; ++i)
		adjacencyMatrix[i][i] = false;

	unsigned int numberVisited = 0;
	bool visited[n];
	for (i = 0; i < n; ++i)
		visited[i] = false;
	depthFirstSearch(0, numberVisited, visited, adjacencyMatrix, n);

	for (i = 0; i < n; ++i)
		delete[] adjacencyMatrix[i];
	delete[] adjacencyMatrix;

	return numberVisited == n;
}

void Constraints::depthFirstSearch(unsigned int toVisit,
		unsigned int &visitedCount, bool* visited,
		const bool* const * adjacencyMatrix, unsigned int matrixSize) {
	visited[toVisit] = true;
	++visitedCount;

	for (unsigned int i = 0; i < matrixSize; ++i)
		if (!visited[i] && adjacencyMatrix[toVisit][i])
			depthFirstSearch(i, visitedCount, visited, adjacencyMatrix, matrixSize);
}

bool Constraints::ensureInsideContainer(
		const std::map<unsigned int, bool> &atomGroupsInitialized,
		Structure &structure, bool debug) const {
	if (NULL == m_pfCubeLWH) {
		return true;
	}
	unsigned int n = structure.getNumberOfAtomGroups();
	unsigned int i, j;
	COORDINATE3 minCoordinates, maxCoordinates;
	COORDINATE3 coordinateSpans; // maxs - mins
	COORDINATE3 shiftCoordinates;
	const FLOAT SOME_BIG_NUMBER = 1e100;
	const COORDINATE4* const* coordinates = structure.getAtomCoordinates();
	const FLOAT* coordinate;

	for (i = 0; i < 3; ++i) {
		minCoordinates[i] = SOME_BIG_NUMBER;
		maxCoordinates[i] = -SOME_BIG_NUMBER;
	}

	const unsigned int* atomGroupIndices = structure.getAtomGroupIndices();
	const AtomGroup* atomGroups = structure.getAtomGroups();
	std::map<unsigned int, bool>::const_iterator it;
	for (it = atomGroupsInitialized.begin(); it != atomGroupsInitialized.end(); it++) {
		for (i = atomGroupIndices[it->first], n = i + atomGroups[it->first].getNumberOfAtoms(); i < n; ++i) {
			coordinate = *coordinates[i];
			for (j = 0; j < 3; ++j) {
				if (coordinate[j] < minCoordinates[j])
					minCoordinates[j] = coordinate[j];
				if (coordinate[j] > maxCoordinates[j])
					maxCoordinates[j] = coordinate[j];
			}
		}
	}

	bool shifting = false;
	for (i = 0; i < 3; ++i) {
		if (debug) {
			printf("In ensureInsideContainer: max[%u] = %lf, min[%u] = %lf, half cube LWH = %lf, initialized = ",
					i, maxCoordinates[i], i, minCoordinates[i], *m_pfHalfCubeLWH);
			for (it = atomGroupsInitialized.begin(); it != atomGroupsInitialized.end(); it++)
				printf(" %u", it->first);
			printf("\n");
		}
		coordinateSpans[i] = maxCoordinates[i] - minCoordinates[i];
		if (maxCoordinates[i] > *m_pfHalfCubeLWH || minCoordinates[i] < -*m_pfHalfCubeLWH) {
			shiftCoordinates[i] = -((maxCoordinates[i] + minCoordinates[i]) * 0.5);
			shifting = true;
		} else {
			shiftCoordinates[i] = 0;
		}
	}

	if (shifting) {
		if (debug) {
			printf("Shift Coordinates: ");
			AtomGroup::printPoint(shiftCoordinates);
			printf("\nBefore Shift:\n");
			structure.print(0);
		}

		AtomGroup* atomGroup;
		const FLOAT* currentCenter;
		COORDINATE3 newCenter;
		for (it = atomGroupsInitialized.begin(); it != atomGroupsInitialized.end(); it++) {
			atomGroup = structure.getAtomGroup(it->first);
			currentCenter = atomGroup->getCenter();
			for (j = 0; j < 3; ++j)
				newCenter[j] = currentCenter[j] + shiftCoordinates[j];
			atomGroup->setCenter(newCenter);
			atomGroup->initRotationMatrix();
			atomGroup->localToGlobal();
		}

		if (debug) {
			printf("After Shift:\n");
			structure.print(0);
		}
	}

	return coordinateSpans[0] <= *m_pfCubeLWH &&
	       coordinateSpans[1] <= *m_pfCubeLWH &&
	       coordinateSpans[2] <= *m_pfCubeLWH;
}

bool Constraints::ensureInsideContainer(Structure &structure) const {
	if (NULL == m_pfCubeLWH) {
		return true;
	}
	COORDINATE3 minCoordinates, maxCoordinates;
	COORDINATE3 coordinateSpans; // maxs - mins
	COORDINATE3 shiftCoordinates;
	const FLOAT SOME_BIG_NUMBER = 1e100;
	unsigned int i, j;
	unsigned int n = structure.getNumberOfAtoms();
	unsigned int m = structure.getNumberOfAtomGroups();
	const COORDINATE4* const* coordinates = structure.getAtomCoordinates();
	const FLOAT* coordinate;

	for (i = 0; i < 3; ++i) {
		minCoordinates[i] = SOME_BIG_NUMBER;
		maxCoordinates[i] = -SOME_BIG_NUMBER;
	}

	for (i = 0; i < n; ++i) {
		coordinate = *coordinates[i];
		for (j = 0; j < 3; ++j) {
			if (coordinate[j] < minCoordinates[j])
				minCoordinates[j] = coordinate[j];
			if (coordinate[j] > maxCoordinates[j])
				maxCoordinates[j] = coordinate[j];
		}
	}

	bool shifting = false;
	for (i = 0; i < 3; ++i) {
		coordinateSpans[i] = maxCoordinates[i] - minCoordinates[i];
		if (maxCoordinates[i] > *m_pfHalfCubeLWH || minCoordinates[i] < -*m_pfHalfCubeLWH) {
			shiftCoordinates[i] = -((maxCoordinates[i] + minCoordinates[i]) * 0.5);
			shifting = true;
		} else {
			shiftCoordinates[i] = 0;
		}
	}

	if (shifting) {
		AtomGroup* atomGroup;
		const FLOAT* currentCenter;
		COORDINATE3 newCenter;
		for (i = 0; i < m; ++i) {
			atomGroup = structure.getAtomGroup(i);
			currentCenter = atomGroup->getCenter();
			for (j = 0; j < 3; ++j)
				newCenter[j] = currentCenter[j] + shiftCoordinates[j];
			atomGroup->setCenter(newCenter);
			atomGroup->initRotationMatrix();
			atomGroup->localToGlobal();
		}
	}

	return coordinateSpans[0] <= *m_pfCubeLWH &&
	       coordinateSpans[1] <= *m_pfCubeLWH &&
	       coordinateSpans[2] <= *m_pfCubeLWH;
}

