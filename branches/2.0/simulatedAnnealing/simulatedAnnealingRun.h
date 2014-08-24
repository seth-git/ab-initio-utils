
#ifndef __SIMULATED_ANNEALING_RUN_H_
#define __SIMULATED_ANNEALING_RUN_H_

#include "../structure.h"

class SimulatedAnnealing;

class SimulatedAnnealingRun {
public:
	const SimulatedAnnealing* m_sharedData;

	unsigned int m_iEnergyCalculations;
	time_t m_tPrevElapsedSeconds; // Time taken by previous runs
	unsigned int m_iIteration;
	bool m_bRunComplete;

	Structure* m_pStructure;
	FLOAT m_fTemperature;
	FLOAT m_fPerturbationsPerIteration;
	FLOAT m_fRotationRadians;
	FLOAT m_fTranslationVectorLength;

	SimulatedAnnealingRun(const SimulatedAnnealing* sharedData, Structure* pStructure);

	void init();

	bool loadResume(const rapidxml::xml_node<>* pResumeElem,
			const Strings* messages);
	bool saveResume(rapidxml::xml_document<> &doc,
			rapidxml::xml_node<>* pSimElem, const Strings* messages);

	static bool iterationComparator(const SimulatedAnnealingRun* a, const SimulatedAnnealingRun* b) { return a->m_iIteration > b->m_iIteration; }

protected:
	time_t getTotalElapsedSeconds();

	static const unsigned int s_resumeMinOccurs[];
};

#endif
