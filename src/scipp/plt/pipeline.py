class Pipeline(list):
    pass

    def run(self, model):
        out = model
        for step in self:
            out = step.func(out, step.values())
        return out
