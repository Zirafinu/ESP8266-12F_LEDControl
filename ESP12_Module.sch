v 20130925 2
C 40000 40000 0 0 0 title-B.sym
C 41200 49300 1 0 0 bridge-1.sym
{
T 41400 50300 5 10 1 1 0 0 1
refdes=PowerSupply
T 41400 50700 5 10 0 0 0 0 1
device=bridge
T 41400 51100 5 10 0 0 0 0 1
symversion=0.1
}
C 43600 47200 1 0 0 pmos-3.sym
{
T 44200 47700 5 10 0 0 0 0 1
device=PMOS_TRANSISTOR
T 43600 47200 5 10 1 1 0 0 1
model-name=IRLZ34N
}
C 44200 46800 1 0 0 pmos-3.sym
{
T 44800 47300 5 10 0 0 0 0 1
device=PMOS_TRANSISTOR
T 44200 46800 5 10 1 1 0 0 1
model-name=IRLZ34N
}
C 45100 46400 1 0 0 pmos-3.sym
{
T 45700 46900 5 10 0 0 0 0 1
device=PMOS_TRANSISTOR
T 45100 46400 5 10 1 1 0 0 1
model-name=IRLZ34N
}
C 50200 48300 1 0 0 resistor-2.sym
{
T 50600 48650 5 10 0 0 0 0 1
device=RESISTOR
T 50400 48600 5 10 1 1 0 0 1
refdes=10K
}
C 50200 47900 1 0 0 resistor-2.sym
{
T 50600 48250 5 10 0 0 0 0 1
device=RESISTOR
T 50400 48200 5 10 1 1 0 0 1
refdes=10K
}
C 49400 47600 1 0 0 connector6-2.sym
{
T 50800 50500 5 10 1 1 0 6 1
refdes=Programming Connector
T 49700 50450 5 10 0 0 0 0 1
device=CONNECTOR_6
T 49700 50650 5 10 0 0 0 0 1
footprint=SIP6N
}
C 46400 45700 1 0 0 header16-1.sym
{
T 46450 44650 5 10 0 1 0 0 1
device=HEADER16
T 46800 49000 5 10 1 1 0 0 1
refdes=ESP-12F
}
C 44400 49400 1 0 0 lm7805-1.sym
{
T 46000 50700 5 10 0 0 0 0 1
device=7805
T 45800 50400 5 10 1 1 0 6 1
refdes=LDO 3.3
}
C 43000 49300 1 0 0 dc-dc-4Port.sym
{
T 42900 50250 5 10 1 1 0 0 1
description=SwitchingRegulator
}
N 43000 49400 42400 49400 4
N 42400 49400 42400 49500 4
N 43000 50000 42400 50000 4
N 44200 50000 44400 50000 4
N 44200 49400 46200 49400 4
N 46000 50000 51100 50000 4
{
T 49000 50000 5 10 1 1 0 0 1
netname=3.3V
}
N 49400 49600 46200 49600 4
{
T 49000 49600 5 10 1 1 0 0 1
netname=GND
}
C 41600 45600 1 0 0 connector4-2_r.sym
{
T 42300 47500 5 10 1 1 0 6 1
refdes=LED-Connector
T 41900 47650 5 10 0 0 0 0 1
device=CONNECTOR_4
T 41900 47850 5 10 0 0 0 0 1
footprint=SIP4N
}
N 44700 47600 44700 49400 4
N 44700 47600 45600 47600 4
N 45600 47600 45600 47200 4
N 44100 48000 44700 48000 4
N 42600 47200 44100 47200 4
{
T 42700 47200 5 10 1 1 0 0 1
netname=Red
}
N 44700 46800 42600 46800 4
{
T 42700 46800 5 10 1 1 0 0 1
netname=Green
}
N 45600 46400 42600 46400 4
{
T 42700 46400 5 10 1 1 0 0 1
netname=Blue
}
N 42600 46000 42700 46000 4
{
T 42700 46000 5 10 1 1 0 0 1
netname=12V
}
N 42700 46000 42700 50000 4
N 46400 45900 46200 45900 4
N 46200 45250 46200 49600 4
N 47800 45900 48000 45900 4
N 48000 45250 48000 50000 4
N 47800 46300 47800 46500 4
N 47800 46500 45100 46500 4
N 47800 46700 47800 46900 4
N 47800 46900 44200 46900 4
N 47800 47100 47800 47300 4
N 47800 47300 43600 47300 4
N 49200 47000 49200 48000 4
{
T 49000 48000 5 10 1 1 0 0 1
netname=Flash
}
N 46400 48700 46400 49200 4
N 46400 49200 49400 49200 4
{
T 49000 49200 5 10 1 1 0 0 1
netname=Rx
}
N 46400 48300 46300 48300 4
N 46300 48300 46300 48800 4
N 46300 48800 49400 48800 4
{
T 49000 48800 5 10 1 1 0 0 1
netname=Tx
}
N 47800 47900 48050 47900 4
N 47800 48400 50200 48400 4
{
T 49000 48400 5 10 1 1 0 0 1
netname=RST
}
N 47800 48400 47800 48700 4
N 51100 48000 51100 50000 4
N 50200 48000 49200 48000 4
N 43600 47300 43600 47400 4
N 44200 46900 44200 47000 4
N 45100 46500 45100 46600 4
N 46400 46700 46400 46600 4
N 46400 46600 48050 46600 4
N 46400 47100 46400 47000 4
N 46400 47000 49200 47000 4
C 46700 45050 1 0 0 capacitor-1.sym
{
T 46900 45750 5 10 0 0 0 0 1
device=CAPACITOR
T 46900 45550 5 10 1 1 0 0 1
refdes=C?
T 46900 45950 5 10 0 0 0 0 1
symversion=0.1
}
N 47600 45250 48000 45250 4
N 46700 45250 46200 45250 4
C 48050 46500 1 0 0 resistor-2.sym
{
T 48450 46850 5 10 0 0 0 0 1
device=RESISTOR
T 48250 46800 5 10 1 1 0 0 1
refdes=10K
}
C 45300 45800 1 0 0 resistor-2.sym
{
T 45700 46150 5 10 0 0 0 0 1
device=RESISTOR
T 45500 46100 5 10 1 1 0 0 1
refdes=10K
}
C 48050 47800 1 0 0 resistor-2.sym
{
T 48450 48150 5 10 0 0 0 0 1
device=RESISTOR
T 48250 48100 5 10 1 1 0 0 1
refdes=10K
}
N 48950 46600 48950 47900 4
N 48000 47300 48950 47300 4
N 46400 46300 45300 46300 4
N 45300 46300 45300 45900 4
