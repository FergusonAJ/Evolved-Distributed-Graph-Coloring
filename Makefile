dirs := World                        #The directories we will copy into MABE prior to compiling

strippedDir = $(notdir $(dir2))
copyHelper = cp ./$(dir)/$(strippedDir) ./MABE/$(dir)/$(strippedDir) -r
copyDir = $(foreach dir2, $(wildcard $(dir)/*), $(copyHelper)) 
rmHelper = rm ./MABE/$(dir)/$(strippedDir) -r
rmDir = $(foreach dir2, $(wildcard $(dir)/*), $(rmHelper)) 



all: copySource
	cp ./buildOptions.txt ./MABE/
	cd MABE; \
	python3 pythonTools/mbuild.py -p 8; \
	cp ./mabe ../mabe

.PHONY: clean
clean: 
	cd MABE; \
	python3 pythonTools/mbuild.py -c -nc
	rm ./mabe
	rm ./MABE/mabe
	$(MAKE) cleanSource

.PHONY: copySource
copySource:
	$(foreach dir, $(dirs), $(copyDir))

.PHONY: cleanSource
cleanSource:
	$(foreach dir, $(dirs), $(rmDir))
