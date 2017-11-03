This build assumes the SDK to be located in this directory and to be called

	SaleaeAnalyzerSdk-1.1.23	

Unzipping the SaleaeAnalyzerSdk-1.1.23.zip distribution in this directory
should do the trick. I norder to compile Windows version unzip the SDK into
the following directories:
- ../sdk/include - for headers (*.hpp)
- ../sdk/lib     - for libraris

Files and Directories:

source
	Actual source code; including a simple Makefile.

What's new:
2017.11.03 - v0.9.2:
	- bit-decoder rewritten
	- better support for RESET detection

2017.10.31 - v0.9.0.1:
	- ISO 7816 state machine to parse ATR, optional PPS and T1 transmission
	- full 7816 compliant ATR parser, including negotiable and specific mode detection
	- T1 frame parser with I/S/R block decoder
	- checksum validation - LRC
	
How to build it:
	- XCode build environment for above source.
	- VC2017 build environment for above source.

Licenses.txt
	License information.

README.txt
	This file

SaleaeAnalyzerSdk-1.1.25
	Not distrubuted; but expected name and location of the SDK. The makefile
	and X-Code build files have hardcoded paths to the include.
	Please download it from: http://support.saleae.com/hc/en-us/categories/200077184-sdks-automation-betas
