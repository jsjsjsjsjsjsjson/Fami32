#ifndef NOTE2FREQ_H
#define NOTE2FREQ_H

#include <stdint.h>
#include <math.h>

const float note2period_table[128] = {
    13680.94272561715f,   // 8.175798915643707 Hz
    12913.034286292184f,  // 8.661957218027252 Hz
    12188.225235921856f,  // 9.177023997418988 Hz
    11504.096591678097f,  // 9.722718241315029 Hz
    10858.36513780666f,   // 10.300861153527183 Hz
    10248.875805606802f,  // 10.913382232281373 Hz
    9673.594481089842f,   // 11.562325709738575 Hz
    9130.601216312782f,   // 12.249857374429663 Hz
    8618.083821730539f,   // 12.978271799373287 Hz
    8134.331818181819f,   // 13.75 Hz
    7677.730728323877f,   // 14.567617547440307 Hz
    7246.756688464496f,   // 15.433853164253883 Hz
    6839.971362808575f,   // 16.351597831287414 Hz
    6456.017143146092f,   // 17.323914436054505 Hz
    6093.612617960928f,   // 18.354047994837977 Hz
    5751.5482958390485f,  // 19.445436482630058 Hz
    5428.68256890333f,    // 20.601722307054366 Hz
    5123.937902803401f,   // 21.826764464562746 Hz
    4836.297240544921f,   // 23.12465141947715 Hz
    4564.800608156391f,   // 24.499714748859326 Hz
    4308.5419108652695f,  // 25.956543598746574 Hz
    4066.6659090909093f,  // 27.5 Hz
    3838.3653641619376f,  // 29.13523509488062 Hz
    3622.8783442322497f,  // 30.86770632850775 Hz
    3419.4856814042873f,  // 32.70319566257483 Hz
    3227.508571573046f,   // 34.64782887210901 Hz
    3046.306308980465f,   // 36.70809598967594 Hz
    2875.2741479195242f,  // 38.890872965260115 Hz
    2713.8412844516643f,  // 41.20344461410875 Hz
    2561.468951401701f,   // 43.653528929125486 Hz
    2417.6486202724604f,  // 46.2493028389543 Hz
    2281.9003040781945f,  // 48.999429497718666 Hz
    2153.770955432635f,   // 51.91308719749314 Hz
    2032.8329545454546f,  // 55.0 Hz
    1918.6826820809688f,  // 58.27047018976124 Hz
    1810.9391721161248f,  // 61.7354126570155 Hz
    1709.2428407021437f,  // 65.40639132514966 Hz
    1613.254285786523f,   // 69.29565774421802 Hz
    1522.6531544902325f,  // 73.41619197935188 Hz
    1437.1370739597621f,  // 77.78174593052023 Hz
    1356.4206422258321f,  // 82.4068892282175 Hz
    1280.2344757008505f,  // 87.30705785825097 Hz
    1208.3243101362302f,  // 92.4986056779086 Hz
    1140.4501520390972f,  // 97.99885899543733 Hz
    1076.3854777163176f,  // 103.82617439498628 Hz
    1015.9164772727273f,  // 110.0 Hz
    958.8413410404844f,   // 116.54094037952248 Hz
    904.9695860580622f,   // 123.47082531403103 Hz
    854.1214203510718f,   // 130.8127826502993 Hz
    806.1271428932615f,   // 138.59131548843604 Hz
    760.8265772451161f,   // 146.8323839587038 Hz
    718.0685369798811f,   // 155.56349186104046 Hz
    677.7103211129162f,   // 164.81377845643496 Hz
    639.6172378504252f,   // 174.61411571650194 Hz
    603.6621550681151f,   // 184.9972113558172 Hz
    569.7250760195487f,   // 195.99771799087463 Hz
    537.6927388581588f,   // 207.65234878997256 Hz
    507.45823863636366f,  // 220.0 Hz
    478.9206705202422f,   // 233.08188075904496 Hz
    451.9847930290311f,   // 246.94165062806206 Hz
    426.5607101755359f,   // 261.6255653005986 Hz
    402.56357144663076f,  // 277.1826309768721 Hz
    379.91328862255807f,  // 293.6647679174076 Hz
    358.53426848994053f,  // 311.1269837220809 Hz
    338.3551605564581f,   // 329.6275569128699 Hz
    319.3086189252126f,   // 349.2282314330039 Hz
    301.33107753405756f,  // 369.9944227116344 Hz
    284.36253800977437f,  // 391.99543598174927 Hz
    268.3463694290794f,   // 415.3046975799451 Hz
    253.22911931818183f,  // 440.0 Hz
    238.9603352601211f,   // 466.1637615180899 Hz
    225.49239651451555f,  // 493.8833012561241 Hz
    212.78035508776796f,  // 523.2511306011972 Hz
    200.78178572331538f,  // 554.3652619537442 Hz
    189.45664431127904f,  // 587.3295358348151 Hz
    178.76713424497026f,  // 622.2539674441618 Hz
    168.67758027822904f,  // 659.2551138257398 Hz
    159.1543094626063f,   // 698.4564628660078 Hz
    150.16553876702878f,  // 739.9888454232688 Hz
    141.68126900488718f,  // 783.9908719634985 Hz
    133.6731847145397f,   // 830.6093951598903 Hz
    126.11455965909092f,  // 880.0 Hz
    118.98016763006055f,  // 932.3275230361799 Hz
    112.24619825725777f,  // 987.7666025122483 Hz
    105.89017754388398f,  // 1046.5022612023945 Hz
    99.89089286165769f,   // 1108.7305239074883 Hz
    94.22832215563952f,   // 1174.6590716696303 Hz
    88.88356712248513f,   // 1244.5079348883237 Hz
    83.83879013911452f,   // 1318.5102276514797 Hz
    79.07715473130315f,   // 1396.9129257320155 Hz
    74.58276938351439f,   // 1479.9776908465376 Hz
    70.34063450244359f,   // 1567.981743926997 Hz
    66.33659235726985f,   // 1661.2187903197805 Hz
    62.55727982954546f,   // 1760.0 Hz
    58.990083815030275f,  // 1864.6550460723597 Hz
    55.6230991286289f,    // 1975.533205024496 Hz
    52.44508877194199f,   // 2093.004522404789 Hz
    49.445446430828845f,  // 2217.4610478149766 Hz
    46.614161077819766f,  // 2349.31814333926 Hz
    43.941783561242566f,  // 2489.0158697766474 Hz
    41.419395069557254f,  // 2637.02045530296 Hz
    39.03857736565158f,   // 2793.825851464031 Hz
    36.791384691757195f,  // 2959.955381693075 Hz
    34.67031725122179f,   // 3135.9634878539946 Hz
    32.668296178634925f,  // 3322.437580639561 Hz
    30.77863991477273f,   // 3520.0 Hz
    28.995041907515137f,  // 3729.3100921447194 Hz
    27.31154956431445f,   // 3951.066410048992 Hz
    25.722544385970995f,  // 4186.009044809578 Hz
    24.222723215414423f,  // 4434.922095629953 Hz
    22.807080538909883f,  // 4698.63628667852 Hz
    21.470891780621283f,  // 4978.031739553295 Hz
    20.209697534778627f,  // 5274.04091060592 Hz
    19.01928868282579f,   // 5587.651702928062 Hz
    17.895692345878597f,  // 5919.91076338615 Hz
    16.835158625610894f,  // 6271.926975707989 Hz
    15.834148089317463f,  // 6644.875161279122 Hz
    14.889319957386364f,  // 7040.0 Hz
    13.997520953757572f,  // 7458.620184289437 Hz
    13.155774782157218f,  // 7902.132820097988 Hz
    12.361272192985497f,  // 8372.018089619156 Hz
    11.611361607707211f,  // 8869.844191259906 Hz
    10.903540269454938f,  // 9397.272573357044 Hz
    10.235445890310642f,  // 9956.06347910659 Hz
    9.604848767389317f,   // 10548.081821211836 Hz
    9.009644341412892f,   // 11175.303405856126 Hz
    8.447846172939299f,   // 11839.8215267723 Hz
    7.917579312805451f    // 12543.853951415975 Hz
};

#define BASE_FREQ_HZ 440.0f
#define FCPU_HZ 1789773.0f

float note2period(uint8_t midi_note, bool mode) {
    if (mode)
        return note2period_table[midi_note & 127];
    else
        return (FCPU_HZ / (16.0f * (BASE_FREQ_HZ * exp2f((midi_note - 69) / 12.0f)))) - 1.0f;
}

float period2freq(float period) {
    return FCPU_HZ / (16.0f * (period + 1.0f));
}

#endif