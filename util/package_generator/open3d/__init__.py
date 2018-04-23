# Copyright (c) 2018, Open3D community
# Redistribution and use is allowed according to the terms of the MIT license.

import importlib
from .open3d import *

globals().update(importlib.import_module('open3d.open3d').__dict__)
