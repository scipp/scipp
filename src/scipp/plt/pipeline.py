class Pipeline(list):
    pass

    def run(self, model):
        out = model
        for step in self:
            # out = step.func(out, step.values())
            out = step(out)
        return out
