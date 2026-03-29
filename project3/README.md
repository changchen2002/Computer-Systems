# CS 5600 Project 3

## Group Information
Group 2  
Byunghyun Ko, Chang Chen, Yuchong Zhang

## Compilation

```
make
```

## Running Tests

Run a specific policy with input_file.txt:

```bash
./scheduler sjf   < testing.txt
./scheduler rr    < testing.txt
./scheduler mlfq  < testing.txt
```

To run all three:

```bash
make full-test
```
