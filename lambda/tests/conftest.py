import os
import sys

# Make lambda/ importable as top-level modules (as they are in the Lambda runtime).
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
