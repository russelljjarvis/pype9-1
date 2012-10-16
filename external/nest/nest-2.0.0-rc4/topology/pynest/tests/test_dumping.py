"""
Tests for topology hl_api dumping functions.
"""

import unittest
import nest
import nest.topology as topo
import sys

class PlottingTestCase(unittest.TestCase):

    def test_DumpNodes(self):
        """Test dumping nodes."""
        ldict = {'elements': 'iaf_neuron', 'rows': 3, 'columns':3,
                 'extent': [2., 2.], 'edge_wrap': True}
        nest.ResetKernel()
        l = topo.CreateLayer(ldict)
        topo.DumpLayerNodes(l, 'test_DumpNodes.out.lyr')
        self.assertTrue(True)
        
    def test_DumpNodes2(self):
        """Test dumping nodes, two layers."""
        ldict = {'elements': 'iaf_neuron', 'rows': 3, 'columns':3,
                 'extent': [2., 2.], 'edge_wrap': True}
        nest.ResetKernel()
        l = topo.CreateLayer(ldict)
        topo.DumpLayerNodes(l*2, 'test_DumpNodes2.out.lyr')
        self.assertTrue(True)

    def test_DumpConns(self):
        """Test dumping connections."""
        ldict = {'elements': 'iaf_neuron', 'rows': 3, 'columns':3,
                 'extent': [2., 2.], 'edge_wrap': True}
        cdict = {'connection_type': 'divergent', 'mask': {'circular': {'radius': 1.}}}
        nest.ResetKernel()
        l = topo.CreateLayer(ldict)
        topo.ConnectLayers(l, l, cdict)
        
        topo.DumpLayerConnections(l, 'static_synapse', 'test_DumpConns.out.cnn')
        self.assertTrue(True)
        
    def test_DumpConns2(self):
        """Test dumping connections, 2 layers."""
        ldict = {'elements': 'iaf_neuron', 'rows': 3, 'columns':3,
                 'extent': [2., 2.], 'edge_wrap': True}
        cdict = {'connection_type': 'divergent', 'mask': {'circular': {'radius': 1.}}}
        nest.ResetKernel()
        l = topo.CreateLayer(ldict)
        topo.ConnectLayers(l, l, cdict)
        
        topo.DumpLayerConnections(l*2, 'static_synapse', 'test_DumpConns2.out.cnn')
        self.assertTrue(True)

def suite():

    suite = unittest.makeSuite(PlottingTestCase,'test')
    return suite


if __name__ == "__main__":

    runner = unittest.TextTestRunner(verbosity=2)
    runner.run(suite())
    
    import matplotlib.pyplot as plt
    plt.show()
