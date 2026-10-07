import os
import sys
import pytest
from iloabot_uds import UnifiedDataService
from unittest.mock import patch, MagicMock

def test_unified_data_service():
    # Create a mock for the UnifiedDataService class
    with patch('iloabot_uds.UnifiedDataService') as MockUnifiedDataService:
        # Create an instance of the mock
        mock_service = MockUnifiedDataService.return_value
        
        # Set up the mock to return a specific value when a method is called
        mock_service.get_data.return_value = {'key': 'value'}
        
        # Call the method and assert the expected behavior
        result = mock_service.get_data()
        assert result == {'key': 'value'}
        
        # Verify that the method was called once
        mock_service.get_data.assert_called_once(
    )