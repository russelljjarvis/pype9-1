"""
UnitTests for multithreaded pynest
"""

import unittest
import nest
import sys


class ThreadTestCase(unittest.TestCase):
    """Multiple threads """

    def test_Threads(self):
        """Multiple threads"""

        # Test if we have a thread-enabled NEST
        nest.sr("statusdict /have_pthreads get")
        if not nest.spp(): return

        nest.ResetKernel()
        self.assertEqual(nest.GetKernelStatus()['local_num_threads'],1)

        nest.SetKernelStatus({'local_num_threads':8})
        n=nest.Create('iaf_neuron',8)
        st = nest.GetStatus(n,'vp')
        st.sort()        
        self.assertEqual(st,[0, 1, 2, 3, 4, 5, 6, 7])


    def test_ThreadsFindConnections(self):
        """FindConnections with threads"""

        # Test if we have a thread-enabled NEST
        nest.sr("statusdict /have_pthreads get")
        if not nest.spp(): return

        nest.ResetKernel()
        nest.SetKernelStatus({'local_num_threads':8})
        pre = nest.Create("iaf_neuron")
        post = nest.Create("iaf_neuron", 6)

        nest.DivergentConnect(pre, post)

        conn = nest.FindConnections(pre)
        targets = nest.GetStatus(conn, "target")
        
        self.assertEqual(targets, post)


    def test_ThreadsGetEvents(self):
        """ Gathering events across threads """

        # Test if we have a thread-enabled NEST
        nest.sr("statusdict /have_pthreads get")
        if not nest.spp(): return

        threads = [1,2,4,8]

        n_events_sd = []
        n_events_vm = []

        N       = 128
        Simtime = 1000.

        for t in threads:

            nest.ResetKernel()
            nest.SetKernelStatus({'local_num_threads': t})

            n  = nest.Create('iaf_psc_alpha', N, {'I_e':2000.}) # force a lot of spike events
            sd = nest.Create('spike_detector')
            vm = nest.Create('voltmeter')

            nest.ConvergentConnect(n,sd)
            nest.DivergentConnect(vm,n)

            nest.Simulate(Simtime)

            n_events_sd.append(nest.GetStatus(sd, 'n_events')[0])
            n_events_vm.append(nest.GetStatus(vm, 'n_events')[0])

        ref_vm = N*(Simtime-1)
        ref_sd = n_events_sd[0]

        # could be done more elegantly with any(), ravel(),
        # but we dont want to be dependent on numpy et al
        [ self.assertEqual(x,ref_vm) for x in n_events_vm]
        [ self.assertEqual(x,ref_sd) for x in n_events_sd]


      

def suite():

    suite = unittest.makeSuite(ThreadTestCase,'test')
    return suite


if __name__ == "__main__":

    runner = unittest.TextTestRunner(verbosity=2)
    runner.run(suite())
