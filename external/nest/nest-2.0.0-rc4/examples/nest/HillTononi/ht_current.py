"""
A small example using the ht_neuron.

The neuron is driven by DC current, which is alternated
between depolarizing and hyperpolarizing. Hyperpolarization
intervals become increasingly longer.

Once simulation is completed, membrane potential and
threshold, as well as intrinsic currents
are shown.
"""

import nest
import numpy as np
import matplotlib.pyplot as pl

nest.ResetKernel()

# create neuron
nrn = nest.Create('ht_neuron', params = {'NaP_g_peak': 1.0})

# get receptor ID information, so we can connect to the 
# different synapses
receptors = nest.GetStatus(nrn)[0]['receptor_types']

# create multimeter and configure it to record all information
# we want at 0.1ms resolution
mm = nest.Create('multimeter')
nest.SetStatus(mm, {'interval': 0.1,
                    'record_from': ['V_m', 'Theta',  # membrane potential, threshold
                                    # intrinsic currents
                                    'I_NaP', 'I_KNa', 'I_T', 'I_h'
                                    ]})

# create DC generator
dc = nest.Create('dc_generator')

# connect generator
nest.Connect(dc, nrn)

# connect multimeter
nest.Connect(mm, nrn)

# some parameters
t_dep = 20
t_hyp = [20, 40, 80, 160, 320, 640, 1280]

t_switch = [0]

# simulate
for t in t_hyp:

    # depolarize for 200ms, amplitude chosen to depolarize to
    nest.SetStatus(dc, {'amplitude': 10.})
    nest.Simulate(t_dep)
    t_switch.append(nest.GetStatus([0], 'time')[0])

    # hyperpolarize for time t, amplitude chosen to hyperpolarize to
    nest.SetStatus(dc, {'amplitude': -30.})
    nest.Simulate(t)
    t_switch.append(nest.GetStatus([0], 'time')[0])

# cut off after last burst 
Tend = 1800

# extract data from multimeter 
events = nest.GetStatus(mm)[0]['events']
t = events['times'];  # time axis

pl.clf()
vax = pl.subplot(211)
vlines = vax.plot(t, events['V_m'], 'b', t, events['Theta'], 'g')
pl.legend(vlines, ['V_m', 'Theta'])
vax.set_ylabel('Membrane potential [mV]')
vax.set_ylim([-100, 40])
pl.title('ht_neuron driven by DC current (dark background: depolarizing)')

for k in xrange(len(t_switch)-1):
    vax.add_patch(pl.Rectangle([t_switch[k], -100], t_switch[k+1]-t_switch[k], 140,
                               ec='none', fc='w' if k%2 else '0.7'))

pl.xlim([0, Tend])

iax=pl.subplot(212)
pl.plot(t, events['I_h'], 'maroon', t, events['I_T'], 'orange',
        t, events['I_NaP']/5, 'crimson', t, events['I_KNa'], 'aqua')
pl.legend(['I_h', 'I_T', 'I_NaP/5', 'I_KNa'], loc='lower right')
pl.ylabel('Current [pA]')
pl.xlabel('Time [ms]')
pl.ylim([-25, 20])
pl.xlim([0, Tend])
for k in xrange(len(t_switch)-1):
    iax.add_patch(pl.Rectangle([t_switch[k], -25], t_switch[k+1]-t_switch[k], 45,
                               ec='none', fc='w' if k%2 else '0.7'))

pl.show()
