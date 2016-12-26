### Ab Initio Utils

This program performs calculations on small molecules or atomic clusters using ab initio techniques. It features a batch processor that reads geometries from quantum chemistry output files or generates them randomly. It then recalculates their energies on a Linux Cluster with MPI. This code was written to support global minimum searching and/or molecular dynamics calculations. Since performing such calculations efficiently in parallel is non-trivial, this code base is intended to simplify the process.

The program uses XML which serves two purposes. First, it simplifies the input file, since XML can describe a complex request concisely. Second, the results are also stored in XML. In fact, the program's entire data structures are written in XML making them [serializable](https://en.wikipedia.org/wiki/Serialization). This allows the program to more easily send messages between MPI processes. The program was written in C++ and uses a [Rapid XML](http://rapidxml.sourceforge.net/) parser which was designed for memory efficiency and speed.

Thus far, the software can create input files for GAMESS-US, GAMESS-UK, Gaussian, and NWChem. It uses [cclib](http://cclib.github.io/) to read output files in the following formats: ADF, Firefly, GAMESS-US, GAMESS-UK, Gaussian, Jaguar, Molpro, NWChem, ORCA, Psi, and QChem. This software represents a partial rewrite of the [Atomic Global Minimum Locator](http://sourceforge.net/projects/atomicglobalmin/).

### Input Files

This section discussions the xml input file and provides examples. The complete list of input file options are described in the [agml.xsd](https://raw.githubusercontent.com/seth-git/ab-initio-utils/master/xsd/agml.xsd) file which is in [XSD format](http://www.w3schools.com/schema/).

Below is an example batch input file that reads GAMESS output files and performs a new set of calculations on their geometries using NWChem.

    <?xml version="1.0" encoding="utf-8"?>
    <agml version="2.0"
          xmlns="http://sourceforge.net/projects/atomicglobalmin/"
          xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
          xsi:schemaLocation="http://sourceforge.net/projects/atomicglobalmin/ agml.xsd">
    	<batch>
    		<setup>
    			<structuresTemplate>
    				<seed>
    					<directory path="/some/directory/with/output/files" type="GAMMESS-US"/>
    				</seed>
    			</structuresTemplate>
    		</setup>
    		<energy>
    			<external method="NWChem">
    				<resultsDirectory path="/some/directory"/>
    				<header><![CDATA[
    title "Nitrogen cc-pvdz SCF geometry optimization"
    geometry  
      #atoms
    end
    basis
      n library cc-pvdz
    end
    task scf optimize
    				]]></header>
    			</external>
    		</energy>
    	</batch>
    </agml>

The above example reads all the files with the "gamout" file extension from the specified directory. If you need to change the file extension associated with GAMESS-US files, it's located at the top of externalEnergy/gamessUS.cc.

The header is the NWChem input file. The "#atoms" string will be replaced with a geometry.

Below is another example that generates random structures.

    <?xml version="1.0" encoding="utf-8"?>
    <agml version="2.0"
          xmlns="http://sourceforge.net/projects/atomicglobalmin/"
          xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
          xsi:schemaLocation="http://sourceforge.net/projects/atomicglobalmin/ agml.xsd">
    	<batch>
    		<setup>
    			<structuresTemplate constraints="const">
    				<structureTemplate>
    					<moleculeTemplate>
    						<![CDATA[
    C -2.646636 -1.062747 -0.826172
    H -2.119982 -2.001828 -0.71049
    H -3.641961 -1.101236 -1.253241
    C -2.111713 0.099834 -0.464456
    H -2.678377 1.017862 -0.603423
    C -0.750835 0.322832 0.142508
    O -0.067373 -1.001838 0.105605
    C 0.095247 1.231857 -0.794368
    H -0.461842 2.164024 -0.930859
    H 0.171239 0.73664 -1.769275
    C -0.831198 0.863314 1.563463
    H -1.313463 0.140349 2.22324
    H 0.159542 1.102554 1.943679
    H -1.43132 1.776562 1.552151
    O 0.761868 -1.229237 1.100385
    O 1.355158 1.567601 -0.273791
    H 1.981122 0.839765 -0.438313
    ]]>
    					</moleculeTemplate>
    					<moleculeTemplate number="2">
    						<![CDATA[
    H 0.757540797781 0 0.587079636589
    H -0.757540797781 0 0.587079636589
    O 0 0 0
    ]]>
    					</moleculeTemplate>
    					<atomTemplate Z="H"/>
    				</structureTemplate>
    				<linear number="50"/>
    				<planar number="50"/>
    				<threeD number="100"/>
    			</structuresTemplate>
    		</setup>
    		<constraints name="const">
    			<cube size="10"/>
    			<atomicDistances>
    				<min value="0.7"/>
    				<min value="0.9" Z1="O" Z2="H"/>
    				<max value="2.7"/>
    			</atomicDistances>
    		</constraints>
    		<energy>
    			<external method="Gaussian" wallTime="24:00:00">
    				<temporaryDirectory path="/optional/directory" />
    				<resultsDirectory path="/full/path" maxFiles="10"/>
    				<header><![CDATA[
    % mem=2000MB
    # B3LYP/6-311++G(d,p) scfcon=4 scfcyc=300 scf=direct
    
    1 2
    !atoms
    				]]></header>
    			</external>
    		</energy>
    		<results orderBy="energy" rmsDistance="0.00001" maxSize="1000" />
    	</batch>
    </agml>

The first moleculeTemplate element has an Isoprene Hydroxyalkyl Peroxy Radical. The second has a water molecule with the number attribute indicating there are two of them. The atomTemplate element specifies hydrogen, so total there are four atomic groups.

The program will perform translation and rotation of these four groups. It will generate 200 structures: 50 linear, 50 planar, and 100 three dimensional. The linear structures have their centers of mass in a line, while the planer structures have their centers of mass in a plane.

The structuresTemplate element has a constraints attribute with the value "const". This value matches the name of the constraints element below it. Constraints include a cube container of length, width, and height 10, a general minimum atomic distance constraint of 0.7, a general maximum distance constraint of 2.7, and a minimum distance constraint between hydrogen and oxygen of 0.9. Units are in angstroms.

This example also has a wallTime. This optional attribute and has the format hours, minutes, seconds HH::MM:SS. If specified, it will stop the calculation after the specified time and store the results calculated thus far in the xml file. This can be useful if using a public cluster on which you have limited time, and where you want clean program termination. You can also stop the run manually at any time by creating a blank "stop" file inside the resultsDirectory.

The optional temporaryDirectory element can be used to specify a local hard drive that is local to each node and is not accessible by other nodes. After calculations are performed here, the results files are copied to the directory specified via the resultsDirectory element which must be on a network drive. The resultsDirectory element has an optional maxFiles attribute which indicates that no more than 10 files should be stored in that directory.

The header element stores the Gaussian input file. The geometry will replace the "!atoms" text.

At the end is a results element. As calculations finish, structures will be stored in this element with their energy values. The results element has three optional attributes. The first is orderBy which can have the values "energy", "filePrefix", and "id". The "energy" value is the default if no orderBy attribute is specified. When rewriting the xml file, defaults are always omitted. The filePrefix is essentially the file name without the file extension, and the id is just a number assigned to each structure. The second attribute is root-mean-square (RSM) distance. It specifies that all structures must be at least this distance from one another. When two structures are closer than this, the one that comes later in the results list will be removed. Since orderBy="energy", the structure with higher energy is removed. For a description of how RMS distance is calculated on two structures see:

Call, S. T.; Boldyrev, A. I., Zubarev, D. Y. Global minimum structure searches via particle swarm optimization. J. Comput. Chem. 2007, 28, 1177–1186.

The third attribute on the results element is maxSize. It specifies that no more than 1000 structures should be stored in the results element in the xml file. 

### Installation Instructions

1. Install needed packages
2. Compile the program and run the unit tests

Step 1: The following packages are needed by this C++ program:
  * g++
  * python
  * scons
  * mpi-default-bin
  * mpi-default-dev

On many Linux systems, you can install the above packages by typing the Linux command:

    apt get your_package_name

Or you can use the 'Synaptic Package Manager' which is available on many Linux distributions.

Step 2: After downloading the program, extract the contained aiu folder using a Linux archive manager, or by typing the following in a Linux terminal:

    gunzip aiu.tar.gz
    tar -xf aiu.tar

Then, navigate into the directory:

    cd aiu

Then compile the program by typing:

    scons

You may see warning messages. If so, this is OK. If you see error messages, this is NOT OK, and it likely means you do not have the right packages installed (see step 1). Also note that older versions of g++ may not work and that you may need a more recent version. The program has been tested and compiles with g++ 4.6.1 and later versions.

Download [cclib](http://cclib.github.io/) and follow the instructions to install it. Copy the aiu/externalEnergy/atomicGlobalMin.py file to the directory where you installed cclib.

Navigate to the aiu/test directory and type 'scons' to compile the unit tests, then type './test' to run them. If all the tests pass, you have successfully installed the program.

To run a batch input file type '..' to navigate out of the test directory. Then type the following:

    ./aiu xml/LJ7_Batch.xml

This generates 100 random Lennard Jones structures with 7 noble gas atoms, calculates their energy values, and stores them back in the xml file. To perform the same run again, you must first delete the resume and result elements from the xml/LJ7_Batch.xml file.

To run the same job in parallel on two processors, type the following:

    mpirun -np 2 aiu xml/LJ7_Batch.xml

