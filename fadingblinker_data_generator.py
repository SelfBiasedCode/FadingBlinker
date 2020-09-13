import math


class GammaTableCalc:
    def __init__(self, bits_in=8, bits_out=16, top=0x8000, gamma=2.2, off_cycles=20, on_cycles=20):
        self.bits_in = bits_in
        self.bits_out = bits_out
        self.top = top
        self.off_cycles = off_cycles
        self.on_cycles = on_cycles
        self.gamma = gamma
        self.output = []

    def calc(self):
        # calculate parameters
        max_index = math.floor(math.pow(2, self.bits_in) - 1)
        max_value = self.top

        # calculate values
        for index in range(0, max_index):
            value = math.floor(max_value * math.pow(index / max_index, self.gamma))
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
        result += "\nstatic const uint{0}_t PROGMEM fadingblinker_data[] =\n{{\n".format(self.bits_out)
        result += "//{0} timer values\n".format(math.floor(math.pow(2, self.bits_in)))

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
                result += "\n"
                wraparound_counter = 0

        # add other constants
        result += "\n// Timer TOP value\n"
        result += "\t{0},".format(self.top)

        result += "\n// Off cycles \n"
        result += "\t{0},".format(self.off_cycles)

        result += "\n// On cycles \n"
        result += "\t{0}".format(self.on_cycles)

        # add tail
        result += "\n};\n\n"
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
