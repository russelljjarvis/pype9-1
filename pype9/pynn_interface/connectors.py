"""
  Author: Thomas G. Close (tclose@oist.jp)
  Copyright: 2012-2014 Thomas G. Close.
  License: This file is part of the "NineLine" package, which is released under
           the MIT Licence, see LICENSE for details.
"""
from __future__ import absolute_import
from abc import ABCMeta
import quantities
import nineml.user
import pyNN.connectors
import pype9.pynn_interface.random
# import pype9.pynn_interface.projections
# from pype9.pynn_interface.expression import create_anonymous_function
# from pype9.pynn_interface.expression.structure import _PositionBasedExpression
from pype9.exceptions import Pype9ProjToCloneNotCreatedException
from .projection import Projection


class Connector(object):

    __metaclass__ = ABCMeta

    @classmethod
    def _convert_params(cls, nineml_params, rng):
        """
        Converts parameters from lib9ml objects into values with 'quantities'
        units and or random distributions
        """
        converted_params = {}
        for name, p in nineml_params.iteritems():
            if p.unit == 'dimensionless':
                conv_param = p.value
            elif p.unit:
                conv_param = quantities.Quantity(p.value, p.unit)
            elif p.value in ('True', 'False'):
                conv_param = bool(p.value)
            elif isinstance(p.value, str):
                conv_param = p.value
            elif isinstance(p.value, nineml.user.Reference):
                try:
                    conv_param = Projection.created_projections[p.value.name]
                except KeyError:
                    raise Pype9ProjToCloneNotCreatedException
            elif isinstance(p.value, nineml.user.RandomDistribution):
                RandomDistribution = getattr(
                    pype9.pynn_interface.random,
                    p.value.definition.componentclass.name)
                conv_param = RandomDistribution(p.value.parameters, rng)
            elif isinstance(p.value, nineml.user.AnonymousFunction):
                conv_param = create_anonymous_function(p.value)
            else:
                raise Exception("Unrecognised child '{}' of type '{}'"
                                .format(p.value, type(p.value)))
            converted_params[cls.nineml_translations[name]] = conv_param
        return converted_params

    def __init__(self, nineml_params, rng=None):
        # Sorry if this feels a bit hacky (i.e. relying on the pyNN class being
        # the third class in the MRO), I thought of a few ways to do this but
        # none were completely satisfactory.
        PyNNClass = self.__class__.__mro__[2]
        assert (PyNNClass.__module__.startswith('pyNN') and
                PyNNClass.__module__.endswith('connectors'))
        PyNNClass.__init__(self, **self._convert_params(nineml_params, rng))


class PositionBasedProbabilityConnector(
        Connector, pyNN.connectors.IndexBasedProbabilityConnector):

    """
    For each pair of pre-post cells, the connection probability depends on an
    function of the displacement between them.

    Takes any of the standard :class:`Connector` optional arguments and, in
    addition:

        `expression`:
            a function that takes a source position and a target position array
            and calculates a probability matrix from them.
        `source_structure`, `target_structure`:
            the part of the source and target cells to use as the reference
            points. This allows multiple reference points on the cell to be
            used, eg. soma, dendritic/axonal branch points.  If a cell only has
            one set of positions then they do not need to be specified
            (typically a soma)
        `allow_self_connections`:
            if the connector is used to connect a Population to itself, this
            flag determines whether a neuron is allowed to connect to itself,
            or only to other neurons in the Population.
        `rng`:
            an :class:`RNG` instance used to evaluate whether connections exist
    """

    nineml_translations = {'allowSelfConnections': 'allow_self_connections',
                           'probabilityExpression': 'expression',
                           'sourceStructure': 'source_structure',
                           'targetStructure': 'target_structure'}

    parameter_names = ('allow_self_connections', 'expression',
                       'source_structure', 'target_structure')

    def __init__(self, nineml_params, rng):
        conv_params = self._convert_params(nineml_params, rng)
        # The branch names are not actually needed here but are just included
        # for the automatic PyNN description function.
        self.source_structure = conv_params['source_structure']
        self.target_structure = conv_params['source_structure']
        # Initialise the index-based probability connector with position-based
        # expression
        pyNN.connectors.IndexBasedProbabilityConnector.__init__(self,
            _PositionBasedExpression(
                expression=conv_params['expression'],
                source_structure=conv_params['source_structure'],
                target_structure=conv_params['target_structure']),
            allow_self_connections=conv_params['allow_self_connections'],
            rng=rng)


class AllToAllConnector(Connector, pyNN.connectors.AllToAllConnector):

    nineml_translations = {'allowSelfConnections': 'allow_self_connections'}


class FixedProbabilityConnector(Connector,
                                pyNN.connectors.FixedProbabilityConnector):

    nineml_translations = {'allowSelfConnections': 'allow_self_connections',
                           'probability': 'p_connect'}


class FixedNumberPostConnector(
        Connector, pyNN.connectors.FixedNumberPostConnector):

    nineml_translations = {
        'allowSelfConnections': 'allow_self_connections', 'number': 'n'}


class FixedNumberPreConnector(
        Connector, pyNN.connectors.FixedNumberPreConnector):

    nineml_translations = {
        'allowSelfConnections': 'allow_self_connections', 'number': 'n'}


class OneToOneConnector(Connector, pyNN.connectors.OneToOneConnector):

    nineml_translations = {}


class SmallWorldConnector(Connector, pyNN.connectors.SmallWorldConnector):

    nineml_translations = {'allowSelfConnections': 'allow_self_connections',
                           'degree': 'degree', 'rewiring': 'rewiring',
                           'numberOfConnections': 'n_connections'}


class CloneConnector(Connector, pyNN.connectors.CloneConnector):

    nineml_translations = {'projection': 'reference_projection'}
