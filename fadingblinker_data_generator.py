import math


class GammaTableCalc:
    def __init__(self, minimum=0x05, top=0x4000, gamma=2.2, off_cycles=20, on_cycles=20, flash_cycles = 20, tone_frequency_hz=440):
        bits_in = 8
        self.indices = math.floor(math.pow(2, bits_in))
        self.store_in_flash = False
        self.min_value = minimum
        self.top = top
        self.gamma = gamma
        self.tone_freq = tone_frequency_hz
        self.off_cycles = off_cycles
        self.on_cycles = on_cycles
        self.flash_cycles = flash_cycles
        self.output = []

    def build_header(self):
        return "// Data Container\n" \
               "struct fadingblinker_data_struct\n" \
               "{\n" \
               f"\tuint16_t pwmData[{self.indices}];\n" \
               "\tuint16_t timerTop;\n" \
               "\tuint8_t holdOffCycles;\n" \
               "\tuint8_t holdOnCycles;\n" \
               "\tuint8_t flashCycles;\n" \
               "\tuint16_t buzzerFreq;\n" \
               "};\n"

    def calc(self):
        # calculate parameters
        max_index = self.indices - 1
        max_value = self.top

        # calculate values
        for index in range(0, max_index):
            value = math.floor(max_value * math.pow(index / max_index, self.gamma))
            # clamp if necessary
            if value < self.min_value:
                value = self.min_value
            self.output.append(value)

        return self.output

    def output_to_string(self, print_to_stdout=False):
        # if data has not been calculated yet, do so
        if not self.output:
            self.calc()

        # write file head
        result = "/* PWM values and constants for FadingBlinker */\n\n"
        result += "#ifndef FADINGBLINKER_DATA_H\n"
        result += "#define FADINGBLINKER_DATA_H\n"
        result += "\n"
        result += self.build_header()
        result += "\n// Generated Data\n"
        result += "static const fadingblinker_data_struct fadingblinker_data"
        if self.store_in_flash:
            result += " PROGMEM"
        result += " =\n{\n"
        result += "\tpwmData: {\n\t"

        # add gamma values
        wraparound_max = 10
        wraparound_counter = 0
        for value in self.output:
            result += "\t{0},".format(value)

            # add extra tab for small numbers
            if value < 100:
                result += "\t"
            wraparound_counter += 1
            if wraparound_counter >= wraparound_max:
                result += "\n\t"
                wraparound_counter = 0
        result += "\n\t},\n"

        # add other constants
        result += "\ttimerTop:"
        result += "\t\t{0},\n".format(self.top)

        result += "\tholdOffCycles:"
        result += "\t\t{0},\n".format(self.off_cycles)
        result += "\tholdOnCycles:"
        result += "\t\t\t{0},\n".format(self.on_cycles)
        result += "\tflashCycles:"
        result += "\t{0},\n".format(self.flash_cycles)
        result += "\tbuzzerFreq:"
        result += "\t\t{0}\n".format(self.tone_freq)

        # add tail
        result += "};\n\n"
        result += "#endif\n"

        # return result and print
        if print_to_stdout:
            print(result)
        return result

    def output_to_file(self, path):
        with open(path, "w+") as file:
            content = self.output_to_string()
            file.write(content)


# entry point for standalone execution
if __name__ == "__main__":
    generator = GammaTableCalc()
    generator.output_to_file("fadingblinker_data.hpp")
    print("fadingblinker_data.hpp generated.")
