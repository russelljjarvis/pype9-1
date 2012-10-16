'''
NEST Topology Module Example

Create layer of 4x3 elements compose of one
pyramidal cell and one interneuron, visualize

BCCN Tutorial @ CNS*09
Hans Ekkehard Plesser, UMB/Simula
'''

import nest
import nest.topology as topo
import pylab
import random

nest.ResetKernel()

nest.CopyModel('iaf_neuron', 'pyr')
nest.CopyModel('iaf_neuron', 'in')
ctx = topo.CreateLayer({'columns': 4, 'rows': 3,
                        'extent': [2.0, 1.5],
                        'elements': ['pyr', 'in']})

nest.PrintNetwork()

nest.PrintNetwork(2)

nest.PrintNetwork(2, ctx)

# extract position information
ppyr = pylab.array(zip(*[topo.GetPosition([n])[0] for n in nest.GetLeaves(ctx)[0] 
                         if nest.GetStatus([n],'model')[0]=='pyr']))
pin  = pylab.array(zip(*[topo.GetPosition([n])[0] for n in nest.GetLeaves(ctx)[0] 
                         if nest.GetStatus([n],'model')[0]=='in']))
# plot
pylab.clf()
pylab.plot(ppyr[0]-0.05, ppyr[1]-0.05, 'bo', markersize=20, label='Pyramidal', zorder=2)
pylab.plot(pin [0]+0.05, pin [1]+0.05, 'ro', markersize=20, label='Interneuron', zorder=2)
pylab.plot(ppyr[0],ppyr[1],'o',markerfacecolor=(0.7,0.7,0.7),
           markersize=60,markeredgewidth=0,zorder=1,label='_nolegend_')

# beautify
pylab.axis([-1.0, 1.0, -0.75, 0.75])
pylab.axes().set_aspect('equal', 'box')
pylab.axes().set_xticks((-0.75, -0.25, 0.25, 0.75))
pylab.axes().set_yticks((-0.5, 0, 0.5))
pylab.grid(True)
pylab.xlabel('4 Columns, Extent: 1.5')
pylab.ylabel('3 Rows, Extent: 1.0')
pylab.legend(numpoints=1)

# pylab.savefig('ctx_2n.png')

