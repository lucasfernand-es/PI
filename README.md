
# Parallel approximation of PI in C using Processes

> Forked from `https://github.com/matheustavares/pi`

Here is a parallel C code that approximates PI using Riemann integration.
We do that calculating an aproximation for the integral of `f(x) = sqrt(1 - x^2)`, with x in `[0, 1]`, which corresponds to one quarter of a unitary circle's area. As we know this value can also be calculated as `(pi * radius^2) / 4`, which, in this case, is `pi / 4`. So to get a PI approximation we just have to multiply the value of the integration by 4.

The goal is to show a simple PI approximation strategy using parallel processes in C. It is used Inter Process Communication (IPC) through shared memory so our processes are able to use the same memory segment.


### To compile

You must have `gcc` and `make` installed. Than, run:
```
    $ make
```
(tested on linux only)

### To run

```
    $ ./pi_process NUM_PROCESSOS NUM_PONTOS
```
Where `NUM_PROCESSOS` is the desired number of processes to be used and `NUM_PONTOS` is the number of partitions to be made for the Riemann integration.