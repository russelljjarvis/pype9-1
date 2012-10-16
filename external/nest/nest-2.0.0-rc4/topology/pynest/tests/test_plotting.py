"""
Tests for basic topology hl_api functions.
"""

import unittest
import nest
import nest.topology as topo
import sys
import matplotlib.pyplot as plt

class PlottingTestCase(unittest.TestCase):

    def test_PlotLayer(self):
        """Test plotting layer."""
        ldict = {'elements': 'iaf_neuron', 'rows': 3, 'columns':3,
                 'extent': [2., 2.], 'edge_wrap': True}
        nest.ResetKernel()
        l = topo.CreateLayer(ldict)
        topo.PlotLayer(l)
                
        self.assertTrue(True)

    def test_PlotTargets(self):
        """Test plotting targets."""
        ldict = {'elements': ['iaf_neuron', 'iaf_psc_alpha'], 'rows': 3, 'columns':3,
                 'extent': [2., 2.], 'edge_wrap': True}
        cdict = {'connection_type': 'divergent', 
                 'mask': {'grid': {'rows':2, 'columns':2}}}     
        nest.ResetKernel()
        l = topo.CreateLayer(ldict)
        ian = [gid for gid in nest.GetLeaves(l)[0] if nest.GetStatus([gid], 'model')[0] == 'iaf_neuron']
        ipa = [gid for gid in nest.GetLeaves(l)[0] if nest.GetStatus([gid], 'model')[0] == 'iaf_psc_alpha']
        
        # connect ian -> all using static_synapse
        cdict.update({'sources': {'model': 'iaf_neuron'},
                      'synapse_model': 'static_synapse'})
        topo.ConnectLayers(l, l, cdict)
        for k in ['sources', 'synapse_model']: cdict.pop(k)
        
        # connect ipa -> ipa using stdp_synapse
        cdict.update({'sources': {'model': 'iaf_psc_alpha'}, 'targets': {'model': 'iaf_psc_alpha'},
                      'synapse_model': 'stdp_synapse'})
        topo.ConnectLayers(l, l, cdict)
        for k in ['sources', 'targets', 'synapse_model']: cdict.pop(k)
 
        ctr_ian, ctr_iap = nest.GetLeaves(topo.FindCenterElement(l))[0]
        fig = topo.PlotTargets([ctr_ian], l)
        fig.gca().set_title('Plain call')
        
        self.assertTrue(True)
        
    def test_PlotKernel(self):
        """Test plotting kernels."""
        ldict = {'elements': 'iaf_neuron', 'rows': 3, 'columns':3,
                 'extent': [2., 2.], 'edge_wrap': True}
        nest.ResetKernel()
        l = topo.CreateLayer(ldict)
        f = plt.figure()
        a1 = f.add_subplot(221)
        ctr = topo.FindCenterElement(l)
        topo.PlotKernel(a1, ctr, {'circular': {'radius': 1.}}, {'gaussian': {'sigma':0.2}})
        
        a2 = f.add_subplot(222)
        topo.PlotKernel(a2, ctr, {'doughnut': {'inner_radius': 0.5, 'outer_radius':0.75}})
        
        a3 = f.add_subplot(223)
        topo.PlotKernel(a3, ctr, {'rectangular': {'lower_left': [-.5,-.5], 
                                              'upper_right':[0.5,0.5]}})
        
        self.assertTrue(True)
        
def suite():

    suite = unittest.makeSuite(PlottingTestCase,'test')
    return suite


if __name__ == "__main__":

    runner = unittest.TextTestRunner(verbosity=2)
    runner.run(suite())
    
    import matplotlib.pyplot as plt
    plt.show()
