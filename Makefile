
all: 

	-$(MAKE) -C readline
	-$(MAKE) -C arbprec
	-$(MAKE) -C bc
	
clean:

	$(MAKE) -C readline clean
	$(MAKE) -C arbprec clean
	$(MAKE) -C bc clean

