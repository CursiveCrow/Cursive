import sys

from lsp_harness import main


if __name__ == "__main__":
    sys.argv = [sys.argv[0], "navigation"]
    main()
