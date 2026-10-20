from ecosmart_ml.fixedpoint import div_round, isqrt
from ecosmart_ml import sensor_model as sm


def test_div_round_matches_c_semantics():
    assert div_round(5, 2) == 3
    assert div_round(-5, 2) == -3
    assert div_round(9, 4) == 2
    for n in range(-1000, 1001):
        assert div_round(n, 7) == -div_round(-n, 7)


def test_isqrt():
    assert isqrt(99999999) == 9999
    assert isqrt(2**64 - 1) == 2**32 - 1


def test_sensor_model_matches_firmware_test_vectors():
    # Same expectations as test/test_sensor_math.
    assert sm.adc_to_millivolts(512) == 2502
    assert sm.ph_centi_from_millivolts(2677) == 600
    assert abs(sm.conductivity_centi_from_adc(485) - 901) <= 5
    assert sm.ds18b20_raw_to_centi(-161) == -1006


def test_inverse_models_round_trip():
    assert abs(sm.measure_ph(5.5) - 550) <= 3
    assert abs(sm.measure_conductivity(9.0) - 900) <= 5
    assert sm.measure_temperature(34.0625) == 3406
