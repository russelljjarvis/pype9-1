

class Pype9RuntimeError(Exception):
    pass


class Pype9DimensionError(Pype9RuntimeError):
    pass


class Pype9TypeError(TypeError, Pype9RuntimeError):
    pass


class Pype9ImportError(Pype9RuntimeError):
    pass


class Pype9AttributeError(AttributeError, Pype9RuntimeError):
    pass


class Pype9IrreducibleMorphException(Pype9RuntimeError):
    pass


class Pype9BuildError(Pype9RuntimeError):
    pass


class Pype9UnflattenableSynapseException(Exception):
    pass


class Pype9CouldNotGuessFromDimensionException(Pype9RuntimeError):
    pass


class Pype9NoMatchingElementException(Pype9RuntimeError):
    pass


class Pype9MemberNameClashException(Pype9RuntimeError):
    pass


class Pype9BuildOptionMismatchException(Pype9RuntimeError):
    pass


class Pype9ProjToCloneNotCreatedException(Pype9RuntimeError):

    def __init__(self, orig_proj_id=None):
        self.orig_proj_id = orig_proj_id
