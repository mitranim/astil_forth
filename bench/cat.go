// BOT-GENERATED

package main

import (
	"fmt"
	"io"
	"os"
)

func copyInput(input *os.File, name string) error {
	if _, err := io.Copy(os.Stdout, input); err != nil {
		return fmt.Errorf("%s: %w", name, err)
	}
	return nil
}

func copyFile(name string) error {
	if name == "-" {
		return copyInput(os.Stdin, "stdin")
	}

	input, err := os.Open(name)
	if err != nil {
		return err
	}

	copyErr := copyInput(input, name)
	closeErr := input.Close()
	if copyErr != nil {
		return copyErr
	}
	return closeErr
}

func run(args []string) error {
	if len(args) == 0 {
		return copyInput(os.Stdin, "stdin")
	}

	for _, name := range args {
		if err := copyFile(name); err != nil {
			return err
		}
	}
	return nil
}

func main() {
	if err := run(os.Args[1:]); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
