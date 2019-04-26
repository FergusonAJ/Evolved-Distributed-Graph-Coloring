cores := 1

dirs := World                        #The directories we will copy into MABE prior to compiling

strippedDir = $(notdir $(dir2))
copyHelper = cp ./$(dir)/$(strippedDir) ./MABE/$(dir)/$(strippedDir) -r
copyDir = $(foreach dir2, $(wildcard $(dir)/*), $(copyHelper)) 
rmHelper = - rm ./MABE/$(dir)/$(strippedDir) -r
rmDir = $(foreach dir2, $(wildcard $(dir)/*), $(rmHelper)) 



all:
	$(MAKE) cleanSource
	$(MAKE) copySource
	cp ./buildOptions.txt ./MABE/
	cd MABE; \
	python3 pythonTools/mbuild.py -p $(cores); \
	cp ./mabe ../mabe

debug:
	$(MAKE) cleanSource
	$(MAKE) copySource
	cp ./buildOptions.txt ./MABE/
	cd MABE; \
	python3 pythonTools/mbuild.py -do -p $(cores); \
	cp ./mabe ../mabe

.PHONY: clean
clean: 
	cd MABE; \
	python3 pythonTools/mbuild.py -c -nc
	$(MAKE) cleanSource
	- rm ./mabe
	- rm ./MABE/mabe
	cd MABE; \
	git checkout buildOptions.txt

.PHONY: copySource
copySource:
	$(foreach dir, $(dirs), $(copyDir))

.PHONY: cleanSource
cleanSource:
	$(foreach dir, $(dirs), $(rmDir))
