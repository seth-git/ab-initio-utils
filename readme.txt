Ab Initio Utils

This program performs calculations on small molecules or atomic clusters using ab initio techniques. It features a batch processor that reads geometries from quantum chemistry output files or generates them randomly. It then recalculates their energies on a Linux Cluster with MPI. This code was written to support global minimum searching and/or molecular dynamics calculations. Since performing such calculations efficiently in parallel is non-trivial, this code base is intended to simplify the process.

The program uses XML which serves two purposes. First, it simplifies the input file, since XML can describe a complex request consicely. Second, the results are also stored in XML. In fact, the program's entire data structures can be written with XML which makes them serializable. This allows the program to more easily send messages between MPI processes, facilitating greater efficiency. The program uses a <a href="http://rapidxml.sourceforge.net/">Rapid XML</a> parser which was designed for memory efficiency and speed.

Below is an example batch input file. It reads NWChem output files and performs a new set of calculations on their geometries using Gaussian.
<?xml version="1.0" encoding="utf-8"?>
<agml version="2.0" xmlns="http://sourceforge.net/projects/atomicglobalmin/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://sourceforge.net/projects/atomicglobalmin/ agml.xsd">
	<batch>
		<setup>
			<structuresTemplate>
				<seed>
					<directory path="/some/directory/with/output/files" type="NWChem"/>
				</seed>
			</structuresTemplate>
		</setup>
		<energy>
			<external method="Gaussian">
				<resultsDirectory path="/some/directory"/>
				<header><![CDATA[
% mem=2000MB
# B3LYP/6-311++G(d,p) scfcon=4 scfcyc=300 scf=direct

0 2
!atoms
				]]></header>
			</external>
		</energy>
	</batch>
</agml>

Thus far, the software can create input files for GAMESS-US, GAMESS-UK, Gaussian, and NWChem. It uses <a href="http://cclib.github.io/">cclib</a> to read output files in the following formats: ADF, Firefly(PC GAMESS), GAMESS-US, GAMESS-UK, Gaussian, Jaguar, Molpro, and ORCA. This software represents a partial rewrite of the <a href="http://sourceforge.net/projects/atomicglobalmin/">Atomic Global Minimum Locator</a>.

Installation Instructions

1. Install needed packages
2. Compile the program and run the unit tests

Step 1: The following packages are needed by the batch calculator which is a C++ program. If you installed pso1.5 or 1.6, the only additional package you should need is scons.
	g++
	python
	scons
	mpi-default-bin
	mpi-default-dev
On many Linux systems, you can install the above packages by typing the Linux command:
	apt get your_package_name
Or you can use the 'Synaptic Package Manager' which is available on many Linux distributions.

Step 2: After downloading the batch program, extract the contained batch folder using a Linux archive manager, or by typing the following in a Linux terminal (where X is the version number):
	gunzip aiu.tar.gz
	tar -xf aiu.tar

Then, navigate into the directory: 
    cd batch

Then compile the program by typing: 
	scons

You may see warning messages. If so, this is OK. If you see error messages, this is NOT OK, and it likely means you do not have the right packages installed (see step 1). Also note that older versions of g++ may not work and that you may need a more recent version. The program has been tested and compiles with g++ 4.6.1 and later versions.

Download cclib from http://cclib.sourceforge.net/ . Follow the instructions to install it. Copy the batch/externalEnergy/atomicGlobalMin.py file to the directory where you installed cclib.

Navigate to the batch/test directory and type 'scons' to compile the unit tests, then type './test' to run them. If all the tests pass, you have successfully installed the program.

To run the program type '..' to navigate out of the test directory and into the batch directory. Then type the following:
	./agml xml/LJ7_Batch.xml
	
This generates 100 random LJ7 structures, calculates their energy values, and stores them back in the xml file. To perform the same run again, you must first delete the resume and result elements. To run the same job in parallel on two processors, type the following:
	mpirun -np 2 agml xml/LJ7_Batch.xml

The xml folder contains more example xml files. A full list of the available options is contained in the xsd/agml.xsd file. This file contains the xml schema in xsd format (see: http://www.w3schools.com/schema/ ). The program has been tested with the Lennard Jones potential and Gaussian.
