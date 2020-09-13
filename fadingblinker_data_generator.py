import math


class GammaTableCalc:
    def __init__(self, bits_in=8, bits_out=16, top=0x8000, fill_factor=0.25, gamma=2.2):
        self.bits_in = bits_in
        self.bits_out = bits_out
        self.top = top
        self.fill_factor = fill_factor
        self.gamma = gamma
        self.output = []

    def calc(self):
        # calculate parameters
        max_index = math.floor(math.pow(2, self.bits_in) - 1)
        fillstart = math.floor(max_index * (1 - self.fill_factor))
        max_value = self.top

        # calculate values
        for index in range(0, max_index):
            if index < fillstart:
                value = math.floor(max_value * math.pow((index / fillstart), self.gamma))
            else:
                value = max_value

            # store result
            self.output.append(value)

        return self.output

    def output_to_string(self, print_to_stdout=False):
        # if data has not been calculated yet, do so
        if not self.output:
            self.calc()

        # write file head
        result = ""
        result += "#ifndef FADINGBLINKER_DATA_H\n"
        result += "#define FADINGBLINKER_DATA_H\n\n"
        result += "// this array contains {0} timer values plus the TOP value for the timer\n".format(
            math.floor(math.pow(2, self.bits_in)))
        result += "static const uint{0}_t PROGMEM fadingblinker_data[] = {{".format(self.bits_out)

        # add gamma values
        max_value = math.floor(math.pow(2, self.bits_in) - 1)
        for value in range(0, max_value):
            result += "\t{0},".format(self.output[value])

        # add TOP and tail
        result += "\t{0}\t}};\n\n".format(self.top)
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
