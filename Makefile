THIRD_PARTY=third-party
MAGMA=$(THIRD_PARTY)/magma
QUADRIC=$(THIRD_PARTY)/quadric

basic-graphics-samples:
	$(MAKE) -C $(MAGMA)
	$(MAKE) -C $(QUADRIC)
	$(MAKE) -C 01-clear
	$(MAKE) -C 02-triangle
	$(MAKE) -C 03-vertex-buffer
	$(MAKE) -C 04-vertex-transform
	$(MAKE) -C 05-mesh
	$(MAKE) -C 06-texture
	$(MAKE) -C 07-texture-array
	$(MAKE) -C 08-texture-cube
	$(MAKE) -C 09-texture-volume
	$(MAKE) -C 10.a-render-to-texture
	$(MAKE) -C 10.b-render-to-msaa-texture
	$(MAKE) -C 11-occlusion-query
	$(MAKE) -C 12-alpha-blend
	$(MAKE) -C 13-specialization
	$(MAKE) -C 14-pushconstants
	$(MAKE) -C 15-particles
	$(MAKE) -C 16-immediate-mode
	$(MAKE) -C 17-shadertoy
	$(MAKE) -C 18-compute

magma:
	$(MAKE) -C $(MAGMA)

quadric:	
	$(MAKE) -C $(QUADRIC)

clean:
	$(MAKE) -C $(MAGMA) clean
	$(MAKE) -C $(QUADRIC) clean
	$(MAKE) -C 01-clear clean
	$(MAKE) -C 02-triangle clean
	$(MAKE) -C 03-vertex-buffer clean
	$(MAKE) -C 04-vertex-transform clean
	$(MAKE) -C 05-mesh clean
	$(MAKE) -C 06-texture clean
	$(MAKE) -C 07-texture-array clean
	$(MAKE) -C 08-texture-cube clean
	$(MAKE) -C 09-texture-volume clean
	$(MAKE) -C 10.a-render-to-texture clean
	$(MAKE) -C 10.b-render-to-msaa-texture clean
	$(MAKE) -C 11-occlusion-query clean
	$(MAKE) -C 12-alpha-blend clean
	$(MAKE) -C 13-specialization clean
	$(MAKE) -C 14-pushconstants clean
	$(MAKE) -C 15-particles clean
	$(MAKE) -C 16-immediate-mode clean
	$(MAKE) -C 17-shadertoy clean
	$(MAKE) -C 18-compute clean

