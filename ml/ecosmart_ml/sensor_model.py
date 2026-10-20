"""Python port of src/sensor_math.cpp plus the inverse (physical -> ADC).

Synthetic signals are pushed through the same 10-bit ADC quantisation and
integer conversion the firmware applies, so training data lives on exactly
the value grid the Nano will see.
"""

from .fixedpoint import clamp, div_round

ADC_MAX = 1023
ADC_REF_MV = 5000

PH_NEUTRAL_MV = 2500
PH_MV_PER_PH = 177

COND_SERIES_OHM = 1000
COND_CELL_CONSTANT_DECI = 100


def adc_to_millivolts(adc: int) -> int:
    return div_round(min(adc, ADC_MAX) * ADC_REF_MV, ADC_MAX)


def ph_centi_from_millivolts(mv: int) -> int:
    return clamp(700 + div_round((PH_NEUTRAL_MV - mv) * 100, PH_MV_PER_PH), 0, 1400)


def conductivity_centi_from_adc(adc: int) -> int:
    if adc <= 0:
        return 0
    if adc >= ADC_MAX:
        return 10000
    num = COND_CELL_CONSTANT_DECI * 10000 * adc
    den = COND_SERIES_OHM * (ADC_MAX - adc)
    return min((num + den // 2) // den, 10000)


def ds18b20_raw_to_centi(raw: int) -> int:
    return div_round(raw * 25, 4)


# --- inverse models: physical value -> raw reading the sensor would produce ---

def ph_to_adc(ph: float) -> int:
    mv = PH_NEUTRAL_MV - (ph - 7.0) * PH_MV_PER_PH
    return int(clamp(round(mv * ADC_MAX / ADC_REF_MV), 0, ADC_MAX))


def conductivity_to_adc(ms_per_cm: float) -> int:
    if ms_per_cm <= 0:
        return 0
    # sigma = K * 1000 / R_x  ->  R_x = K * 1000 / sigma (ohm, K in cm^-1)
    r_x = (COND_CELL_CONSTANT_DECI / 10.0) * 1000.0 / ms_per_cm
    return int(clamp(round(ADC_MAX * COND_SERIES_OHM / (COND_SERIES_OHM + r_x)), 0, ADC_MAX))


def temperature_to_raw(deg_c: float) -> int:
    return int(round(deg_c * 16))


def measure_ph(ph: float) -> int:
    return ph_centi_from_millivolts(adc_to_millivolts(ph_to_adc(ph)))


def measure_conductivity(ms_per_cm: float) -> int:
    return conductivity_centi_from_adc(conductivity_to_adc(ms_per_cm))


def measure_temperature(deg_c: float) -> int:
    return ds18b20_raw_to_centi(temperature_to_raw(deg_c))
