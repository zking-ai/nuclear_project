from iloabot_uds import UnifiedDataService
import pytest
import os
import json
import logging
import sys
from unittest.mock import patch MagicMock

source /opt/ros/humble/setup.bash

cd ~/orin_migration/orin/ros2_ws

colcon build --symlink-install --packages-up-to iloabot_uds